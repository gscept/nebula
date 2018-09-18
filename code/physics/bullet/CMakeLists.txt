#-------------------------------------------------------------------------------
# Physics using bullet
#-------------------------------------------------------------------------------

fips_begin_module(physics-bullet)
    fips_vs_warning_level(3)
    fips_include_directories(${CMAKE_CURRENT_SOURCE_DIR})    
    fips_deps(foundation physics)
    fips_deps(Bullet)
       
    fips_dir(. "Bullet")
    fips_files(
        bulletcharacter.h
        bulletcharacter.cc
        bulletbody.cc
        bulletbody.h
        bulletcollider.cc
        bulletcollider.h
        bulletphysicsmesh.cc
        bulletphysicsmesh.h
        bulletphysicsserver.cc
        bulletphysicsserver.h
        bulletscene.cc
        bulletscene.h
        bulletstatic.h
        bulletstatic.cc
        bulletprobe.h
        bulletprobe.cc
        bulletjoint.h
        bulletjoint.cc
        conversion.h
        nebulamotionstate.h
        debugdrawer.h
        debugdrawer.cc
        bullethinge.h
        bulletuniversaljoint.h
        bullethinge.cc
        bulletuniversaljoint.cc
        bulletslider.h
        bulletslider.cc
        bulletpointtopoint.h
        bulletpointtopoint.cc
        raycallbacks.h	
    )    
fips_end_module()
