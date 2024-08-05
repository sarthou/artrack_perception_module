#include "artrack_perception_module/ArTrackPerceptionModule.h"

#include "overworld/Utility/ShellDisplay.h"
#include <unordered_map>

#include <pluginlib/class_list_macros.h>

namespace owds
{

#define TO_HALF_RAD M_PI / 180. / 2.

  ArTrackPerceptionModule::ArTrackPerceptionModule() : PerceptionModuleRosSyncBase("ar_pose_marker", "ar_pose_visible_marker", true),
                                                       sensor_(nullptr),
                                                       ontologies_manipulator_(nullptr),
                                                       onto_(nullptr),
                                                       tf2_listener_(tf_buffer_)
  {
  }

  bool ArTrackPerceptionModule::closeInitialization()
  {
    ontologies_manipulator_ = new onto::OntologiesManipulator();
    std::string robot_name = robot_agent_->getId();
    if (ontologies_manipulator_->add(robot_name) == false)
    {
      // We first try without the wait as its block a long while service is available
      ontologies_manipulator_->waitInit();
      ontologies_manipulator_->add(robot_name);
    }
    onto_ = ontologies_manipulator_->get(robot_name);
    onto_->close();

    min_track_err_ = 0.2; // 20 cm shift

    setSensorPtr();

    return true;
  }

  void ArTrackPerceptionModule::setParameter(const std::string &parameter_name, const std::string &parameter_value)
  {
    if (parameter_name == "min_track_err")
      min_track_err_ = std::stod(parameter_value);
    else if(parameter_name == "sensor_id")
      sensor_id_ = parameter_value;
    else
      ShellDisplay::warning("[ArTrackPerceptionModule] Unkown parameter " + parameter_name);
  }

  bool ArTrackPerceptionModule::perceptionCallback(const ar_track_alvar_msgs::AlvarMarkers &markers,
                                                   const ar_track_alvar_msgs::AlvarVisibleMarkers &visible_markers)
  {
    if(sensor_id_.empty())
    {
      sensor_id_ = markers.header.frame_id;
      setSensorPtr();
    }

    if (robot_agent_ == nullptr)
      return false;
    else if(sensor_ == nullptr)
      return false;
    else if (sensorHasMoved())
      return false;

    std::vector<ar_track_alvar_msgs::AlvarVisibleMarker> valid_visible_markers;
    std::unordered_set<size_t> invalid_main_markers_ids;
    for (const auto &visible_marker : visible_markers.markers)
    {
      geometry_msgs::PoseStamped marker_pose;
      auto old_pose = visible_marker.pose;
      if (old_pose.header.frame_id[0] == '/')
        old_pose.header.frame_id = old_pose.header.frame_id.substr(1);

      try
      {
        tf_buffer_.transform(old_pose, marker_pose, "map", ros::Duration(1.0));
      }
      catch (const tf2::TransformException &ex)
      {
        ShellDisplay::error("[ArTrackPerceptionModule]" + std::string(ex.what()));
        return false;
      }
      catch (...)
      {
        return false;
      }

      if (isInValidArea(Pose(marker_pose)) && visible_marker.confidence < min_track_err_)
        valid_visible_markers.push_back(visible_marker);
      else
        invalid_main_markers_ids.insert(visible_marker.main_id);
    }

    updatePercepts(markers, invalid_main_markers_ids);
    setAllPoiUnseen();

    for (auto &visible_marker : valid_visible_markers)
    {
      if (visible_markers_with_pois_.count(visible_marker.id) == 0 && (ids_map_.find(visible_marker.main_id) != ids_map_.end()))
      {
        // This visible marker has never been seen before (or was not valid) or its entity was not created, let's create its pois
        setPointOfInterest(visible_marker);
        visible_markers_with_pois_.insert(visible_marker.id);
      }
      if (ids_map_.find(visible_marker.main_id) != ids_map_.end())
        percepts_.at(ids_map_[visible_marker.main_id]).setSeen();
    }

    for (const auto &seen_visible_markers : visible_markers.markers)
    {
      // For all the seen marker (even the invalid) if the entity has been created,
      // we said it has been seen if it has been seen just before
      if (ids_map_.find(seen_visible_markers.main_id) != ids_map_.end())
        if (percepts_.at(ids_map_[seen_visible_markers.main_id]).getNbFrameUnseen() < 2)
          percepts_.at(ids_map_[seen_visible_markers.main_id]).setSeen();
    }
    return true;
  }

  bool ArTrackPerceptionModule::sensorHasMoved()
  {
    if (sensor_ == nullptr)
      return true;
    if (sensor_->isLocated() == false)
      return true;
    return sensor_->hasMoved();
  }

  bool ArTrackPerceptionModule::isInValidArea(const Pose& tag_pose)
  {
    auto tag_in_head = tag_pose.transformIn(sensor_->pose());
    return (sensor_->getFieldOfView().hasIn(tag_in_head));
  }

  void ArTrackPerceptionModule::setPointOfInterest(const ar_track_alvar_msgs::AlvarVisibleMarker &visible_marker)
  {
    auto id_it = ids_map_.find(visible_marker.main_id);
    if (id_it == ids_map_.end())
    {
      ShellDisplay::warning("[ArTrackPerceptionModule] tag " + std::to_string(visible_marker.main_id) + " is unknown.");
      return;
    }

    std::string poi_id = "ar_" + std::to_string(visible_marker.id);
    auto prc_obj_it = percepts_.find(id_it->second);

    if (prc_obj_it->second.isLocated() == false)
      return;

    for (const auto &poi : prc_obj_it->second.getPointsOfInterest(this->getModuleName()))
      if (poi.getId() == poi_id)
        return;

    double half_size = visible_marker.size / 100. / 2.; // we also put it in meters

    PointOfInterest p(poi_id);
    Pose sub_pois[5] = {Pose({{0.0, 0.0, 0.0}}, {{0.0, 0.0, 0.0, 1.0}}),
                        Pose({{-half_size, -half_size, 0.0}}, {{0.0, 0.0, 0.0, 1.0}}),
                        Pose({{half_size, -half_size, 0.0}}, {{0.0, 0.0, 0.0, 1.0}}),
                        Pose({{half_size, half_size, 0.0}}, {{0.0, 0.0, 0.0, 1.0}}),
                        Pose({{-half_size, half_size, 0.0}}, {{0.0, 0.0, 0.0, 1.0}})};
    geometry_msgs::PoseStamped map_to_visible_marker_g;
    auto old_pose = visible_marker.pose;
    if (old_pose.header.frame_id[0] == '/')
      old_pose.header.frame_id = old_pose.header.frame_id.substr(1);

    try
    {
      tf_buffer_.transform(old_pose, map_to_visible_marker_g, "map", ros::Duration(1.0));
    }
    catch (const tf2::TransformException &ex)
    {
      ShellDisplay::error("[ArTrackPerceptionModule]" + std::string(ex.what()));
      return;
    }
    catch (...)
    {
      return;
    }

    Pose map_to_visible_marker(map_to_visible_marker_g);
    Pose map_to_marked_percept_object = prc_obj_it->second.pose();

    Pose marker_in_marked_prc_obj = map_to_visible_marker.transformIn(map_to_marked_percept_object);
    for (const auto &sub_poi : sub_pois)
    {
      Pose marked_obj_to_poi = marker_in_marked_prc_obj * sub_poi;
      p.addPoint(marked_obj_to_poi);
    }
    prc_obj_it->second.addPointOfInterest(this->getModuleName(), p);
  }

  void ArTrackPerceptionModule::setAllPoiUnseen()
  {
    for (auto &percept : percepts_)
    {
      percept.second.setAllPoiUnseen(this->getModuleName());
      percept.second.setUnseen();
    }
  }

  void ArTrackPerceptionModule::updatePercepts(const ar_track_alvar_msgs::AlvarMarkers &main_markers,
                                               const std::unordered_set<size_t> &invalid_main_markers_ids)
  {
    for (const auto &main_marker : main_markers.markers)
    {
      bool created = false;
      if (blacklist_ids_.find(main_marker.id) != blacklist_ids_.end())
        continue;
      else if (invalid_main_markers_ids.find(main_marker.id) != invalid_main_markers_ids.end())
      {
        continue;
      }

      auto main_id_it = ids_map_.find(main_marker.id);
      if (main_id_it == ids_map_.end())
      {
        if (createNewPercept(main_marker) == false)
          continue;
        else
          main_id_it = ids_map_.find(main_marker.id);
      }

      auto it_obj_prc = percepts_.find(main_id_it->second);
      it_obj_prc->second.setConfidence(main_marker.confidence);
      std::string frame_id = main_marker.header.frame_id;
      if (frame_id[0] == '/')
        frame_id = frame_id.substr(1);

      geometry_msgs::TransformStamped to_map;
      try
      {
        to_map = tf_buffer_.lookupTransform("map", frame_id, main_marker.header.stamp, ros::Duration(1.0));
      }
      catch (const tf2::TransformException &ex)
      {
        ShellDisplay::error("[ArTrackPerceptionModule]" + std::string(ex.what()));
        return;
      }
      catch (...)
      {
        return;
      }

      geometry_msgs::PoseStamped marker_in_map;
      tf2::doTransform(main_marker.pose, marker_in_map, to_map);
      it_obj_prc->second.updatePose(marker_in_map);
    }
  }

  bool ArTrackPerceptionModule::createNewPercept(const ar_track_alvar_msgs::AlvarMarker &marker)
  {
    auto true_id = onto_->individuals.getFrom("hasArId", "real#" + std::to_string(marker.id));
    if (true_id.size() == 0)
    {
      blacklist_ids_.insert(marker.id);
      ShellDisplay::warning("[ArTrackPerceptionModule] marker " + std::to_string(marker.id) + " was added to the blacklist");
      return false;
    }

    ids_map_[marker.id] = true_id[0];
    Object obj_prc(true_id[0]);

    Shape_t shape = ontology::getEntityShape(onto_, obj_prc.id());
    if (shape.type == SHAPE_NONE)
    {
      shape.type = SHAPE_CUBE;
      shape.color = ontology::getEntityColor(onto_, obj_prc.id(), {1, 0, 0});
      shape.scale = {0.05, 0.05, 0.003};
    }
    obj_prc.setShape(shape);
    obj_prc.setMass(ontology::getEntityMass(onto_, obj_prc.id()));

    auto percept_it = percepts_.insert(std::make_pair(obj_prc.id(), obj_prc)).first;
    percept_it->second.setSensorId(sensor_->id().empty() ? marker.header.frame_id : sensor_->id());
    percept_it->second.setModuleName(this->getModuleName());

    return true;
  }

  void ArTrackPerceptionModule::setSensorPtr()
  {
    if(sensor_id_.empty() == false)
    {
      if(robot_agent_ != nullptr)
      {
        sensor_ = robot_agent_->getSensor(sensor_id_);
        if(sensor_ == nullptr)
        {
          auto sensors_ids = onto_->individuals.getFrom("hasFrameId", sensor_id_);
          if(sensors_ids.empty() == false)
            sensor_ = robot_agent_->getSensor(sensors_ids.front());
          if(sensor_ == nullptr)
          {
            ShellDisplay::warning("[ArTrackPerceptionModule] no sensor find related to \'" + sensor_id_ + "\'. Retry at next frame.");
            sensor_id_ = "";
          }
        }
      }
    }
  }

} // namespace owds

PLUGINLIB_EXPORT_CLASS(owds::ArTrackPerceptionModule, owds::PerceptionModuleBase_<owds::Object>)