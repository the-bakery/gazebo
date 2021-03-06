/*
 * Copyright 2011 Nate Koenig & Andrew Howard
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
#include <sstream>

#include "msgs/msgs.h"

#include "common/Console.hh"
#include "gui/GuiEvents.hh"
#include "math/Quaternion.hh"
#include "common/MouseEvent.hh"

#include "rendering/UserCamera.hh"

#include "transport/Publisher.hh"

#include "gui/BoxMaker.hh"

using namespace gazebo;
using namespace gui;

unsigned int BoxMaker::counter = 0;

BoxMaker::BoxMaker()
: EntityMaker()
{
  this->state = 0;
  this->visualMsg = new msgs::Visual();
  this->visualMsg->mutable_geometry()->set_type(msgs::Geometry::BOX);
  this->visualMsg->mutable_material()->set_script(
      "Gazebo/TurquoiseGlowOutline");
  msgs::Set(this->visualMsg->mutable_pose()->mutable_orientation(),
            math::Quaternion());
}

BoxMaker::~BoxMaker()
{
  this->camera.reset();
  delete this->visualMsg;
}

void BoxMaker::Start(const rendering::UserCameraPtr _camera)
{
  this->camera = _camera;

  std::ostringstream stream;
  stream << "__GZ_USER_box_" << counter++;
  this->visualMsg->set_name(stream.str());

  this->state = 1;
}

void BoxMaker::Stop()
{
  msgs::Request *msg = msgs::CreateRequest("entity_delete",
                                           this->visualMsg->name());

  this->requestPub->Publish(*msg);
  delete msg;

  this->state = 0;
  gui::Events::moveMode(true);
}

bool BoxMaker::IsActive() const
{
  return this->state > 0;
}

void BoxMaker::OnMousePush(const common::MouseEvent &_event)
{
  if (this->state == 0)
    return;

  this->mousePushPos = _event.pressPos;
}

/////////////////////////////////////////////////
void BoxMaker::OnMouseRelease(const common::MouseEvent &_event)
{
  if (this->state == 0)
    return;

  this->state++;

  this->mouseReleasePos = _event.pos;

  if (this->state == 3)
  {
    this->CreateTheEntity();
    this->Stop();
  }
}

/////////////////////////////////////////////////
void BoxMaker::OnMouseMove(const common::MouseEvent &_event)
{
  if (this->state != 2)
    return;

  math::Vector3 p = msgs::Convert(this->visualMsg->pose().position());
  math::Vector3 scale = msgs::Convert(this->visualMsg->geometry().box().size());

  scale.z = (this->mouseReleasePos.y - _event.pos.y)*0.01;
  if (!_event.shift)
    scale.z = rint(scale.z);
  p.z = scale.z/2.0;

  msgs::Set(this->visualMsg->mutable_pose()->mutable_position(), p);
  msgs::Set(this->visualMsg->mutable_geometry()->mutable_box()->mutable_size(),
      scale);

  this->visPub->Publish(*this->visualMsg);
}

/////////////////////////////////////////////////
void BoxMaker::OnMouseDrag(const common::MouseEvent &_event)
{
  if (this->state != 1)
    return;

  math::Vector3 norm(0, 0, 1);
  math::Vector3 p1, p2;

  if (!this->camera->GetWorldPointOnPlane(this->mousePushPos.x,
                                          this->mousePushPos.y,
                                          math::Plane(norm), p1))
  {
    gzerr << "Invalid mouse point\n";
    return;
  }

  p1 = this->GetSnappedPoint(p1);

  if (!this->camera->GetWorldPointOnPlane(
        _event.pos.x, _event.pos.y , math::Plane(norm), p2))
  {
    gzerr << "Invalid mouse point\n";
    return;
  }

  p2 = this->GetSnappedPoint(p2);

  msgs::Set(this->visualMsg->mutable_pose()->mutable_position(), p1);

  math::Vector3 scale = p1-p2;
  math::Vector3 p = msgs::Convert(this->visualMsg->pose().position());

  scale.z = 0.01;
  p.x = p1.x - scale.x/2.0;
  p.y = p1.y - scale.y/2.0;


  msgs::Set(this->visualMsg->mutable_pose()->mutable_position(), p);
  msgs::Set(this->visualMsg->mutable_geometry()->mutable_box()->mutable_size(),
      scale.GetAbs());

  this->visPub->Publish(*this->visualMsg);
}

void BoxMaker::CreateTheEntity()
{
  msgs::Factory msg;

  std::ostringstream newModelStr;

  math::Vector3 p = msgs::Convert(this->visualMsg->pose().position());
  math::Vector3 size = msgs::Convert(this->visualMsg->geometry().box().size());


  newModelStr << "<gazebo version ='1.0'>\
    <model name='custom_user_box" << counter << "_model'>\
    <origin pose='" << p.x << " " << p.y << " " << size.z * 0.5 << " 0 0 0'/>\
    <link name ='link'>\
      <inertial mass ='1.0'>\
          <inertia ixx ='1' ixy ='0' ixz ='0' iyy ='1' iyz ='0' izz ='1'/>\
      </inertial>\
      <collision name ='collision'>\
        <geometry>\
          <box size ='" << size.x << " " << size.y << " " << size.z << "'/>\
        </geometry>\
      </collision>\
      <visual name ='visual' cast_shadows ='true'>\
        <geometry>\
          <box size ='" << size.x << " " << size.y << " " << size.z << "'/>\
        </geometry>\
        <material script ='Gazebo/Grey'/>\
      </visual>\
    </link>\
  </model>\
  </gazebo>";

  msg.set_sdf(newModelStr.str());

  msgs::Request *requestMsg = msgs::CreateRequest("entity_delete",
      this->visualMsg->name());
  this->requestPub->Publish(*requestMsg);
  delete requestMsg;

  this->makerPub->Publish(msg);
  this->camera.reset();
}
