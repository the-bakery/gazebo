SET (files camera.material
           camera.mesh
           chassis.material
           chassis.mesh
           epuck_chassis.mesh
           new_epuck.mesh
           plate.material
           plate.mesh
           ring.material
           ring.mesh
           turret.material
           turret.mesh
)

INSTALL(FILES ${files} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/gazebo-${GAZEBO_VERSION}/Media/models/epuck)
