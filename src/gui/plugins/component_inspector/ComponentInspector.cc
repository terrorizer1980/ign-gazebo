/*
 * Copyright (C) 2019 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/

#include <iostream>
#include <regex>
#include <ignition/common/Console.hh>
#include <ignition/common/Profiler.hh>
#include <ignition/gui/Application.hh>
#include <ignition/gui/MainWindow.hh>
#include <ignition/plugin/Register.hh>

#include "ignition/gazebo/components/AngularAcceleration.hh"
#include "ignition/gazebo/components/AngularVelocity.hh"
#include "ignition/gazebo/components/CastShadows.hh"
#include "ignition/gazebo/components/ChildLinkName.hh"
#include "ignition/gazebo/components/Collision.hh"
#include "ignition/gazebo/components/Factory.hh"
#include "ignition/gazebo/components/Gravity.hh"
#include "ignition/gazebo/components/Joint.hh"
#include "ignition/gazebo/components/Level.hh"
#include "ignition/gazebo/components/Light.hh"
#include "ignition/gazebo/components/LinearAcceleration.hh"
#include "ignition/gazebo/components/LinearVelocity.hh"
#include "ignition/gazebo/components/LinearVelocitySeed.hh"
#include "ignition/gazebo/components/Link.hh"
#include "ignition/gazebo/components/MagneticField.hh"
#include "ignition/gazebo/components/Model.hh"
#include "ignition/gazebo/components/Name.hh"
#include "ignition/gazebo/components/ParentEntity.hh"
#include "ignition/gazebo/components/ParentLinkName.hh"
#include "ignition/gazebo/components/Performer.hh"
#include "ignition/gazebo/components/PerformerAffinity.hh"
#include "ignition/gazebo/components/Pose.hh"
#include "ignition/gazebo/components/PoseCmd.hh"
#include "ignition/gazebo/components/Sensor.hh"
#include "ignition/gazebo/components/Static.hh"
#include "ignition/gazebo/components/Visual.hh"
#include "ignition/gazebo/components/WindMode.hh"
#include "ignition/gazebo/components/World.hh"
#include "ignition/gazebo/EntityComponentManager.hh"
#include "ignition/gazebo/gui/GuiEvents.hh"

#include "ComponentInspector.hh"

namespace ignition::gazebo
{
  class ComponentInspectorPrivate
  {
    /// \brief Model holding all the current components.
    public: TreeModel treeModel;

    /// \brief Entity being inspected. Default to world.
    public: Entity entity{1};

    /// \brief Entity type, such as 'world' or 'model'.
    public: QString type;
  };
}

using namespace ignition;
using namespace gazebo;

//////////////////////////////////////////////////
template<>
void ignition::gazebo::setData(QStandardItem *_item, const math::Pose3d &_data)
{
  _item->setData(QString("Pose3d"), TreeModel::RoleNames().key("dataType"));
  _item->setData(QList({
    QVariant(_data.Pos().X()),
    QVariant(_data.Pos().Y()),
    QVariant(_data.Pos().Z()),
    QVariant(_data.Rot().Roll()),
    QVariant(_data.Rot().Pitch()),
    QVariant(_data.Rot().Yaw())
  }), TreeModel::RoleNames().key("data"));
}

//////////////////////////////////////////////////
template<>
void ignition::gazebo::setData(QStandardItem *_item, const math::Vector3d &_data)
{
  _item->setData(QString("Vector3d"), TreeModel::RoleNames().key("dataType"));
  _item->setData(QList({
    QVariant(_data.X()),
    QVariant(_data.Y()),
    QVariant(_data.Z())
  }), TreeModel::RoleNames().key("data"));
}

//////////////////////////////////////////////////
template<>
void ignition::gazebo::setData(QStandardItem *_item, const std::string &_data)
{
  _item->setData(QString("String"), TreeModel::RoleNames().key("dataType"));
  _item->setData(QString::fromStdString(_data),
      TreeModel::RoleNames().key("data"));
}

//////////////////////////////////////////////////
template<>
void ignition::gazebo::setData(QStandardItem *_item,
    const std::ostringstream &_data)
{
  _item->setData(QString("Raw"), TreeModel::RoleNames().key("dataType"));
  _item->setData(QString::fromStdString(_data.str()),
      TreeModel::RoleNames().key("data"));
}

//////////////////////////////////////////////////
template<>
void ignition::gazebo::setData(QStandardItem *_item, const bool &_data)
{
  _item->setData(QString("Boolean"), TreeModel::RoleNames().key("dataType"));
  _item->setData(_data, TreeModel::RoleNames().key("data"));
}

//////////////////////////////////////////////////
template<>
void ignition::gazebo::setData(QStandardItem *_item, const int &_data)
{
  _item->setData(QString("Integer"), TreeModel::RoleNames().key("dataType"));
  _item->setData(_data, TreeModel::RoleNames().key("data"));
}

//////////////////////////////////////////////////
template<>
void ignition::gazebo::setData(QStandardItem *_item, const double &_data)
{
  _item->setData(QString("Float"), TreeModel::RoleNames().key("dataType"));
  _item->setData(_data, TreeModel::RoleNames().key("data"));
}

/////////////////////////////////////////////////
std::string shortName(const std::string &_typeName)
{
  // Remove namespaces
  auto name = _typeName.substr(_typeName.rfind('.')+1);

  // Split CamelCase
  std::regex reg("(\\B[A-Z])");
  name = std::regex_replace(name, reg, " $1");

  return name;
}

/////////////////////////////////////////////////
TreeModel::TreeModel() : QStandardItemModel()
{
}

/////////////////////////////////////////////////
QStandardItem *TreeModel::AddComponentType(
    ignition::gazebo::ComponentTypeId _typeId)
{
  IGN_PROFILE_THREAD_NAME("Qt thread");
  IGN_PROFILE("TreeModel::AddComponentType");

  auto typeName = QString::fromStdString(
      components::Factory::Instance()->Name(_typeId));

  auto itemIt = this->items.find(_typeId);

  // Existing component item
  if (itemIt != this->items.end())
  {
    return itemIt->second;
  }

  // New component item
  auto item = new QStandardItem(typeName);
  item->setData(QString::fromStdString(shortName(
      typeName.toStdString())), this->roleNames().key("shortName"));
  item->setData(typeName, this->roleNames().key("typeName"));
  item->setData(QString::number(_typeId),
      this->roleNames().key("typeId"));

  this->invisibleRootItem()->appendRow(item);
  this->items[_typeId] = item;
  return item;
}

/////////////////////////////////////////////////
void TreeModel::RemoveComponentType(ignition::gazebo::ComponentTypeId _typeId)
{
  IGN_PROFILE_THREAD_NAME("Qt thread");
  IGN_PROFILE("TreeModel::RemoveComponentType");

  auto itemIt = this->items.find(_typeId);

  // Existing component item
  if (itemIt != this->items.end())
  {
    this->invisibleRootItem()->removeRow(itemIt->second->row());
    this->items.erase(_typeId);
  }
}

/////////////////////////////////////////////////
QHash<int, QByteArray> TreeModel::roleNames() const
{
  return TreeModel::RoleNames();
}

/////////////////////////////////////////////////
QHash<int, QByteArray> TreeModel::RoleNames()
{
  return {std::pair(100, "typeName"),
          std::pair(101, "typeId"),
          std::pair(102, "shortName"),
          std::pair(103, "dataType"),
          std::pair(104, "data")};
}

/////////////////////////////////////////////////
ComponentInspector::ComponentInspector()
  : GuiSystem(), dataPtr(std::make_unique<ComponentInspectorPrivate>())
{
  // Connect model
  ignition::gui::App()->Engine()->rootContext()->setContextProperty(
      "ComponentInspectorModel", &this->dataPtr->treeModel);

  qRegisterMetaType<ignition::gazebo::ComponentTypeId>();
}

/////////////////////////////////////////////////
ComponentInspector::~ComponentInspector() = default;

/////////////////////////////////////////////////
void ComponentInspector::LoadConfig(const tinyxml2::XMLElement *)
{
  if (this->title.empty())
    this->title = "Component inspector";

  ignition::gui::App()->findChild<
      ignition::gui::MainWindow *>()->installEventFilter(this);
}

//////////////////////////////////////////////////
void ComponentInspector::Update(const UpdateInfo &,
    EntityComponentManager &_ecm)
{
  IGN_PROFILE("ComponentInspector::Update");

  auto componentTypes = _ecm.ComponentTypes(this->dataPtr->entity);

  // TODO(louise) Sort so type and name are up top

  // List all components
  for (const auto &typeId : componentTypes)
  {
    // Type components
    if (typeId == components::World::typeId)
    {
      this->SetType("world");
      continue;
    }

    if (typeId == components::Model::typeId)
    {
      this->SetType("model");
      continue;
    }

    if (typeId == components::Link::typeId)
    {
      this->SetType("link");
      continue;
    }

    if (typeId == components::Collision::typeId)
    {
      this->SetType("collision");
      continue;
    }

    if (typeId == components::Visual::typeId)
    {
      this->SetType("visual");
      continue;
    }

    if (typeId == components::Sensor::typeId)
    {
      this->SetType("sensor");
      continue;
    }

    if (typeId == components::Joint::typeId)
    {
      this->SetType("joint");
      continue;
    }

    if (typeId == components::Performer::typeId)
    {
      this->SetType("performer");
      continue;
    }

    if (typeId == components::Level::typeId)
    {
      this->SetType("level");
      continue;
    }

    if (typeId == components::Light::typeId)
    {
      this->SetType("light");
      continue;
    }

    // Add component to list
    QStandardItem *item;
    // TODO(louise) Blocking here is not the best idea
    QMetaObject::invokeMethod(&this->dataPtr->treeModel, "AddComponentType",
        Qt::BlockingQueuedConnection,
        Q_RETURN_ARG(QStandardItem *, item),
        Q_ARG(ignition::gazebo::ComponentTypeId, typeId));

    if (nullptr == item)
    {
      ignerr << "Failed to create item for component type [" << typeId << "]"
             << std::endl;
      continue;
    }

    // Populate component-specific data
    if (typeId == components::AngularAcceleration::typeId)
    {
      auto comp = _ecm.Component<components::AngularAcceleration>(
          this->dataPtr->entity);
      if (comp)
        setData(item, comp->Data());
    }
    else if (typeId == components::AngularVelocity::typeId)
    {
      auto comp = _ecm.Component<components::AngularVelocity>(
          this->dataPtr->entity);
      if (comp)
        setData(item, comp->Data());
    }
    else if (typeId == components::CastShadows::typeId)
    {
      auto comp = _ecm.Component<components::CastShadows>(
          this->dataPtr->entity);
      if (comp)
        setData(item, comp->Data());
    }
    else if (typeId == components::ChildLinkName::typeId)
    {
      auto comp = _ecm.Component<components::ChildLinkName>(
          this->dataPtr->entity);
      if (comp)
        setData(item, comp->Data());
    }
    else if (typeId == components::Gravity::typeId)
    {
      auto comp = _ecm.Component<components::Gravity>(this->dataPtr->entity);
      if (comp)
        setData(item, comp->Data());
    }
    else if (typeId == components::LinearAcceleration::typeId)
    {
      auto comp = _ecm.Component<components::LinearAcceleration>(
          this->dataPtr->entity);
      if (comp)
        setData(item, comp->Data());
    }
    else if (typeId == components::LinearVelocity::typeId)
    {
      auto comp = _ecm.Component<components::LinearVelocity>(
          this->dataPtr->entity);
      if (comp)
        setData(item, comp->Data());
    }
    else if (typeId == components::MagneticField::typeId)
    {
      auto comp = _ecm.Component<components::MagneticField>(
          this->dataPtr->entity);
      if (comp)
        setData(item, comp->Data());
    }
    else if (typeId == components::Name::typeId)
    {
      auto comp = _ecm.Component<components::Name>(this->dataPtr->entity);
      if (comp)
        setData(item, comp->Data());
    }
    else if (typeId == components::ParentEntity::typeId)
    {
      auto comp = _ecm.Component<components::ParentEntity>(this->dataPtr->entity);
      if (comp)
        setData(item, comp->Data());
    }
    else if (typeId == components::ParentLinkName::typeId)
    {
      auto comp = _ecm.Component<components::ParentLinkName>(
          this->dataPtr->entity);
      if (comp)
        setData(item, comp->Data());
    }
    else if (typeId == components::PerformerAffinity::typeId)
    {
      auto comp = _ecm.Component<components::PerformerAffinity>(
          this->dataPtr->entity);
      if (comp)
        setData(item, comp->Data());
    }
    else if (typeId == components::Pose::typeId)
    {
      auto comp = _ecm.Component<components::Pose>(this->dataPtr->entity);
      if (comp)
        setData(item, comp->Data());
    }
    else if (typeId == components::Static::typeId)
    {
      auto comp = _ecm.Component<components::Static>(this->dataPtr->entity);
      if (comp)
        setData(item, comp->Data());
    }
    else if (typeId == components::WindMode::typeId)
    {
      auto comp = _ecm.Component<components::WindMode>(this->dataPtr->entity);
      if (comp)
        setData(item, comp->Data());
    }
    else if (typeId == components::WorldAngularAcceleration::typeId)
    {
      auto comp = _ecm.Component<components::WorldAngularAcceleration>(
          this->dataPtr->entity);
      if (comp)
        setData(item, comp->Data());
    }
    else if (typeId == components::WorldLinearVelocity::typeId)
    {
      auto comp = _ecm.Component<components::WorldLinearVelocity>(
          this->dataPtr->entity);
      if (comp)
        setData(item, comp->Data());
    }
    else if (typeId == components::WorldLinearVelocitySeed::typeId)
    {
      auto comp = _ecm.Component<components::WorldLinearVelocitySeed>(
          this->dataPtr->entity);
      if (comp)
        setData(item, comp->Data());
    }
    else if (typeId == components::WorldPose::typeId)
    {
      auto comp = _ecm.Component<components::WorldPose>(this->dataPtr->entity);
      if (comp)
        setData(item, comp->Data());
    }
    else if (typeId == components::WorldPoseCmd::typeId)
    {
      auto comp = _ecm.Component<components::WorldPoseCmd>(
          this->dataPtr->entity);
      if (comp)
        setData(item, comp->Data());
    }
    // Unknown types use raw data
    // TODO(louise) Enable this once we have a good use case. Right now it just
    // adds garbage with no benefit.
    //else
    //{
    //  auto comp = _ecm.Component(this->dataPtr->entity, typeId);
    //  if (comp)
    //  {
    //    std::ostringstream ostr;
    //    comp->Serialize(ostr);
    //    setData(item, ostr);
    //  }
    //}
  }

  // Remove components no longer present
  for (auto itemIt : this->dataPtr->treeModel.items)
  {
    auto typeId = itemIt.first;
    if (componentTypes.find(typeId) == componentTypes.end())
    {
      QMetaObject::invokeMethod(&this->dataPtr->treeModel, "RemoveComponentType",
          Qt::QueuedConnection,
          Q_ARG(ignition::gazebo::ComponentTypeId, typeId));
    }
  }
}

/////////////////////////////////////////////////
bool ComponentInspector::eventFilter(QObject *_obj, QEvent *_event)
{
  if (_event->type() == ignition::gazebo::gui::events::EntitiesSelected::Type)
  {
    auto selectedEvent =
        reinterpret_cast<gui::events::EntitiesSelected *>(_event);
    if (selectedEvent && !selectedEvent->Data().empty())
    {
      this->SetEntity(*selectedEvent->Data().begin());
    }
  }

  // Standard event processing
  return QObject::eventFilter(_obj, _event);
}

/////////////////////////////////////////////////
int ComponentInspector::Entity() const
{
  return this->dataPtr->entity;
}

/////////////////////////////////////////////////
void ComponentInspector::SetEntity(const int &_entity)
{
  // If nothing is selected, display world properties
  if (_entity == kNullEntity)
  {
    // TODO(anyone) Don't hardcode world entity
    this->dataPtr->entity = 1;
  }
  else
  {
    this->dataPtr->entity = _entity;
  }
  this->EntityChanged();
}

/////////////////////////////////////////////////
QString ComponentInspector::Type() const
{
  return this->dataPtr->type;
}

/////////////////////////////////////////////////
void ComponentInspector::SetType(const QString &_type)
{
  this->dataPtr->type = _type;
  this->TypeChanged();
}

// Register this plugin
IGNITION_ADD_PLUGIN(ignition::gazebo::ComponentInspector,
                    ignition::gui::Plugin)
