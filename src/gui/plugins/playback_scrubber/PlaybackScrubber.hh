/*
 * Copyright (C) 2020 Open Source Robotics Foundation
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

#ifndef IGNITION_GAZEBO_GUI_PLAYBACK_SCRUBBER_HH_
#define IGNITION_GAZEBO_GUI_PLAYBACK_SCRUBBER_HH_

#include <memory>

#include <ignition/gui/Plugin.hh>
#include <ignition/gazebo/gui/GuiSystem.hh>

namespace ignition
{
namespace gazebo
{
  class PlaybackScrubberPrivate;

  /// \brief Provides buttons for adding a box, sphere, or cylinder
  /// to the scene
  class PlaybackScrubber : public ignition::gazebo::GuiSystem
  {
    Q_OBJECT

    /// \brief Constructor
    public: PlaybackScrubber();

    /// \brief Destructor
    public: ~PlaybackScrubber() override;

    // Documentation inherited
    public: void LoadConfig(const tinyxml2::XMLElement *_pluginElem) override;

    // Documentation inherited
    public: void Update(const UpdateInfo &, EntityComponentManager &) override;

    /// \brief Calculate the percentage that `_currentTime`, eg, halfway
    /// through the log would evaluate to 0.50
    /// \param[in] _currentTime The current time the log playback is at
    /// \return The progress as a percentage of how far the log playback
    /// has advanced
    public: double CalculateProgress(const common::Time &_currentTime);

    /// \brief Get the current progress as a percentage of how far the log
    /// playback has advanced
    /// \return The progress as a value from 0 to 1, inclusive
    public slots: double Progress();

    /// \brief Callback in Qt thread when the slider is released.
    /// \param[in] _value The current value of the slider, from 0 to 1,
    /// inclusive
    public slots: void OnDrag(double _value);

    /// \brief Notify that progress has advanced in the log file.
    signals: void newProgress();

    /// \internal
    /// \brief Pointer to private data.
    private: std::unique_ptr<PlaybackScrubberPrivate> dataPtr;
  };
}
}

#endif
