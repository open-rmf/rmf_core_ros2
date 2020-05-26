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

#include "internal_RobotUpdateHandle.hpp"

#include <rmf_traffic_ros2/Time.hpp>

namespace rmf_fleet_adapter {
namespace agv {

//==============================================================================
std::shared_ptr<RobotCommandHandle> RobotContext::command()
{
  return _command_handle.lock();
}

//==============================================================================
std::shared_ptr<RobotUpdateHandle> RobotContext::make_updater()
{
  return RobotUpdateHandle::Implementation::make(shared_from_this());
}

//==============================================================================
Eigen::Vector3d RobotContext::position() const
{
  assert(!_location.empty());
  const auto& l = _location.front();
  if (l.location().has_value())
  {
    const Eigen::Vector2d& p = *l.location();
    return {p[0], p[1], l.orientation()};
  }

  const Eigen::Vector2d& p =
      navigation_graph().get_waypoint(l.waypoint()).get_location();
  return {p[0], p[1], l.orientation()};
}

//==============================================================================
const std::string& RobotContext::map() const
{
  assert(!_location.empty());
  return navigation_graph()
      .get_waypoint(_location.front().waypoint()).get_map_name();
}

//==============================================================================
rmf_traffic::Time RobotContext::now() const
{
  return rmf_traffic_ros2::convert(_node->now());
}

//==============================================================================
const std::vector<rmf_traffic::agv::Plan::Start>& RobotContext::location() const
{
  return _location;
}

//==============================================================================
rmf_traffic::schedule::Participant& RobotContext::itinerary()
{
  return _itinerary;
}

//==============================================================================
const rmf_traffic::schedule::Participant& RobotContext::itinerary() const
{
  return _itinerary;
}

//==============================================================================
auto RobotContext::schedule() const -> const std::shared_ptr<const Snappable>&
{
  return _schedule;
}

//==============================================================================
const rmf_traffic::schedule::ParticipantDescription&
RobotContext::description() const
{
  return _itinerary.description();
}

//==============================================================================
const std::string& RobotContext::name() const
{
  return _itinerary.description().name();
}

//==============================================================================
const rmf_traffic::agv::Graph& RobotContext::navigation_graph() const
{
  return _planner->get_configuration().graph();
}

//==============================================================================
const std::shared_ptr<const rmf_traffic::agv::Planner>&
RobotContext::planner() const
{
  return _planner;
}

//==============================================================================
class RobotContext::NegotiatorSubscription
{
public:

  NegotiatorSubscription(
      std::shared_ptr<RobotContext> context,
      rmf_traffic::schedule::Negotiator* negotiator)
    : _context(context),
      _negotiator(negotiator)
  {
    // Do nothing
  }

  ~NegotiatorSubscription()
  {
    const auto context = _context.lock();
    if (!context)
      return;

    if (context && context->_negotiator == _negotiator)
      context->_negotiator = nullptr;
  }

private:
  std::weak_ptr<RobotContext> _context;
  rmf_traffic::schedule::Negotiator* _negotiator;
};

//==============================================================================
auto RobotContext::set_negotiator(
    rmf_traffic::schedule::Negotiator* negotiator)
-> std::shared_ptr<NegotiatorSubscription>
{
  _negotiator = negotiator;

  return std::make_shared<NegotiatorSubscription>(
        shared_from_this(), negotiator);
}

//==============================================================================
rclcpp::Node& RobotContext::node()
{
  return *_node;
}

//==============================================================================
RobotContext::RobotContext(
  std::shared_ptr<RobotCommandHandle> command_handle,
  std::vector<rmf_traffic::agv::Plan::Start> _initial_location,
  rmf_traffic::schedule::Participant itinerary,
  std::shared_ptr<const Snappable> schedule,
  rmf_traffic::agv::Planner::Configuration configuration,
  rmf_traffic::agv::Planner::Options default_options)
  : _command_handle(std::move(command_handle)),
    _location(std::move(_initial_location)),
    _itinerary(std::move(itinerary)),
    _schedule(std::move(schedule)),
    _planner(std::make_shared<rmf_traffic::agv::Planner>(
               std::move(configuration), std::move(default_options)))
{
  // Do nothing
}

} // namespace agv
} // namespace rmf_fleet_adapter
