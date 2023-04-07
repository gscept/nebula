nebula_begin_module(input)

#get_filename_component(PARENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../ ABSOLUTE)
target_include_directories(input PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
fips_deps(foundation)

fips_dir(input)
fips_files(
	char.h
	gamepad.cc
	gamepad.h
	inputevent.h
	inputhandler.cc
	inputhandler.h
	inputpriority.h
	inputserver.cc
	inputserver.h
	key.cc
	key.h
	keyboard.cc
	keyboard.h
	mouse.cc
	mouse.h
	mousebutton.cc
	mousebutton.h
)
fips_dir(input/base GROUP "input/base")
	fips_files(
		gamepadbase.cc
		gamepadbase.h
		inputserverbase.cc
		inputserverbase.h
		keyboardbase.cc
		keyboardbase.h
		mousebase.cc
		mousebase.h
	)
fips_dir(input/glfw GROUP "input/glfw")
	fips_files(
		glfwinputserver.cc
		glfwinputserver.h
	)
	
nebula_end_module()
