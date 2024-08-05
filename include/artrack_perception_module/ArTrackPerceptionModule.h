#ifndef OWDS_ARTRACKPERCEPTIONMODULE_H
#define OWDS_ARTRACKPERCEPTIONMODULE_H

#include "overworld/BasicTypes/Object.h"
#include "overworld/BasicTypes/Percept.h"
#include "overworld/BasicTypes/Agent.h"
#include "overworld/BasicTypes/Sensors/SensorBase.h"
#include "overworld/Perception/Modules/PerceptionModuleBase.h"

#include "ar_track_alvar_msgs/AlvarMarkers.h"
#include "ar_track_alvar_msgs/AlvarVisibleMarkers.h"

#include "ontologenius/OntologiesManipulator.h"

#include <map>
#include <unordered_set>

#include <tf2_ros/transform_listener.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>

namespace owds
{

  class ArTrackPerceptionModule : public PerceptionModuleRosSyncBase<Object, ar_track_alvar_msgs::AlvarMarkers, ar_track_alvar_msgs::AlvarVisibleMarkers>
  {
  public:
    ArTrackPerceptionModule();

    virtual void setParameter(const std::string &parameter_name, const std::string &parameter_value) override;
    virtual bool closeInitialization() override;

  private:
    Pose last_head_pose_;
    std::string sensor_id_;
    Sensor* sensor_;

    std::map<size_t, std::string> ids_map_;
    std::unordered_set<size_t> blacklist_ids_;
    std::unordered_set<size_t> visible_markers_with_pois_; // The id of the visible markers that we already seen at least once and were valid (and thus, have their pois created)

    onto::OntologiesManipulator *ontologies_manipulator_;
    onto::OntologyManipulator *onto_;

    tf2_ros::Buffer tf_buffer_;
    tf2_ros::TransformListener tf2_listener_;

    double min_track_err_;

    bool perceptionCallback(const ar_track_alvar_msgs::AlvarMarkers &markers,
                            const ar_track_alvar_msgs::AlvarVisibleMarkers &visible_markers) override;

    bool sensorHasMoved();
    bool isInValidArea(const Pose &tag_pose);

    void setPointOfInterest(const ar_track_alvar_msgs::AlvarVisibleMarker &visible_marker);
    void setAllPoiUnseen();
    void updatePercepts(const ar_track_alvar_msgs::AlvarMarkers &main_markers,
                        const std::unordered_set<size_t> &invalid_main_markers_ids);
    bool createNewPercept(const ar_track_alvar_msgs::AlvarMarker &marker);

    void setSensorPtr();
  };

} // namespace owds

#endif // OWDS_ARTRACKPERCEPTIONMODULE_H