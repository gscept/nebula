#-------------------------------------------------------------------------------
# Physics
#-------------------------------------------------------------------------------

fips_begin_module(physics)
    fips_vs_warning_level(3)
    
    fips_deps(foundation)

    if(N_BUILD_BULLET)
        fips_deps(physics-bullet)
    endif()
                
    fips_files(
        physicsserver.cc
        physicsserver.h

        visualdebuggerserver.cc
        visualdebuggerserver.h

        scene.h
        scene.cc

        physicsbody.h
        physicsbody.cc

        physicsobject.h
        physicsobject.cc

        collider.h
        collider.cc

        contactlistener.h
        contactlistener.cc

        staticobject.h
        staticobject.cc

        filterset.h
        filterset.cc

        materialtable.h
        materialtable.cc

        contact.h
        contact.cc

        physicsprobe.h
        physicsprobe.cc

        templates.h
        templates.cc

        joint.h
        joint.cc
        joints.h

        character.h
        character.cc

        #controller.h
        #controller.cc

        #vehicle.h
        #vehicle.cc

        #material.h
        #material.cc
    )
    fips_dir(util)
    fips_files(
        areaimpulse.h
        areaimpulse.cc

        explosionareaimpulse.h
        explosionareaimpulse.cc

        mousegripper.h
        mousegripper.cc
    )

    fips_dir(base)
    fips_files(
        basescene.h
        basescene.cc
        basebody.h
        basebody.cc
        basecollider.h
        basecollider.cc
        basecontact.h
        basecontact.cc
        basecontactlistener.h
        basecontactlistener.cc
        basestatic.h
        basestatic.cc
        baseprobe.h
        baseprobe.cc
        basecharacter.h
        basecharacter.cc
        #base/basecontroller.h
        #base/basecontroller.cc
        #base/basevehicle.h
        #base/basevehicle.cc
        physicsserverbase.h
        physicsserverbase.cc
        basevisualdebuggerserver.h
        basevisualdebuggerserver.cc
        #base/basematerial.h
        #base/basematerial.cc
        basejoint.h
        basejoint.cc
    )    
    fips_dir(joints)
    fips_files(            
        basehinge.h
        basehinge.cc
        baseuniversaljoint.h   
        baseuniversaljoint.cc  
        baseragdolljoint.h   
        baseragdolljoint.cc  
        basechainjoint.h   
        basechainjoint.cc  
        baseslider.h         
        baseslider.cc 			
        basepointtopoint.h       
        basepointtopoint.cc			
        sliderjoint.h
        sliderjoint.cc
        universaljoint.h
        universaljoint.cc
        ragdolljoint.h
        ragdolljoint.cc
        chainjoint.h         
        chainjoint.cc
        hingejoint.h           
        hingejoint.cc 			
        pointtopointjoint.h       
        pointtopointjoint.cc
    )
fips_end_module()
