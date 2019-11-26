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

#include <ignition/msgs/empty.pb.h>

#include <iterator>

#include <ignition/common/Profiler.hh>

#include <ignition/math/Quaternion.hh>
#include <ignition/plugin/Register.hh>

#include "ignition/gazebo/components/Pose.hh"
#include "ignition/gazebo/components/World.hh"

#include "Breadcrumbs.hh"

using namespace ignition;
using namespace gazebo;
using namespace systems;

//////////////////////////////////////////////////
void Breadcrumbs::Configure(const Entity &_entity,
    const std::shared_ptr<const sdf::Element> &_sdf,
    EntityComponentManager &_ecm,
    EventManager &_eventMgr)
{
  this->model = Model(_entity);

  if (!_sdf->HasElement("breadcrumb"))
  {
    ignerr << "<breadcrumb> not set" << std::endl;
    return;
  }

  auto breadcrumb = _sdf->GetElementImpl("breadcrumb");

  if (!breadcrumb->HasElement("sdf"))
  {
    ignerr << "<sdf> not found in <breadcrumb>" << std::endl;
    return;
  }

  auto sdfElem = breadcrumb->GetElementImpl("sdf");
  // We can't load the model directly because it won't go through the SDF
  // validation process, so we first convert to text and call
  // Root::LoadSdfString
  sdf::Errors errors = this->modelRoot.LoadSdfString(sdfElem->ToString(""));
  if (!errors.empty())
  {
    for (const auto &e : errors)
    {
      ignerr << e.Message() << std::endl;
    }
    return;
  }
  if (this->modelRoot.ModelCount() == 0)
  {
    ignerr << "Model not found in <breadcrumb>" << std::endl;
    return;
  }

  this->maxDeployments =
      _sdf->Get<int>("max_deployments", this->maxDeployments).first;

  // Subscribe to commands
  std::string topic{"/model/" + this->model.Name(_ecm) + "/breadcrumbs/" +
                    this->modelRoot.ModelByIndex(0)->Name() + "/deploy"};

  if (_sdf->HasElement("topic"))
    topic = _sdf->Get<std::string>("topic");

  this->node.Subscribe(topic, &Breadcrumbs::OnDeploy, this);

  ignmsg << "Breadcrumbs subscribing to deploy messages on [" << topic << "]"
         << std::endl;

  this->creator = std::make_unique<SdfEntityCreator>(_ecm, _eventMgr);

  this->worldEntity = _ecm.EntityByComponents(components::World());

  this->initialized = true;
}

//////////////////////////////////////////////////
void Breadcrumbs::PreUpdate(const ignition::gazebo::UpdateInfo &,
    ignition::gazebo::EntityComponentManager &_ecm)
{
  IGN_PROFILE("Breadcrumbs::PreUpdate");

  if (this->initialized)
  {
    std::vector<bool> cmds;
    {
      std::lock_guard<std::mutex> lock(this->pendingCmdsMutex);
      std::copy(this->pendingCmds.begin(), this->pendingCmds.end(),
                std::back_inserter(cmds));
      this->pendingCmds.clear();
    }

    auto poseComp = _ecm.Component<components::Pose>(this->model.Entity());

    for (std::size_t i = 0; i < cmds.size(); ++i)
    {
      if (this->maxDeployments < 0 ||
          this->numDeployments < this->maxDeployments)
      {
        sdf::Model modelToSpawn = *this->modelRoot.ModelByIndex(0);
        modelToSpawn.SetName(modelToSpawn.Name() + "_" +
                             std::to_string(this->numDeployments));
        modelToSpawn.SetPose(poseComp->Data() * modelToSpawn.Pose());
        ignmsg << "Deploying " << modelToSpawn.Name() << " at "
               << modelToSpawn.Pose() << std::endl;
        Entity entity = this->creator->CreateEntities(&modelToSpawn);
        this->creator->SetParent(entity, this->worldEntity);
        ++this->numDeployments;
      }
      else
      {
        ignmsg << "Asked to deploy " << this->modelRoot.ModelByIndex(0)->Name()
               << " but the maximum number of deployments has reached the "
               << "limit of " << this->maxDeployments << std::endl;
      }
    }
  }
}

//////////////////////////////////////////////////
void Breadcrumbs::OnDeploy(const msgs::Empty &)
{
  IGN_PROFILE("Breadcrumbs::PreUpdate");
  {
    std::lock_guard<std::mutex> lock(this->pendingCmdsMutex);
    this->pendingCmds.push_back(true);
  }
}

IGNITION_ADD_PLUGIN(Breadcrumbs,
                    ignition::gazebo::System,
                    Breadcrumbs::ISystemConfigure,
                    Breadcrumbs::ISystemPreUpdate)

IGNITION_ADD_PLUGIN_ALIAS(Breadcrumbs, "ignition::gazebo::systems::Breadcrumbs")