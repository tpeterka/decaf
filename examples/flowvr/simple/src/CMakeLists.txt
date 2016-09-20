# TICTAC EXAMPLE 

find_package(Boost COMPONENTS serialization)



# MODULE  BINARIES
if(Boost_FOUND)
	INCLUDE_DIRECTORIES(${FLOWVR_INCLUDE_DIR} ${Boost_INCLUDE_DIRS})

	FOREACH(target put get)
	  ADD_EXECUTABLE(${target} ${target}.cpp)
	# Should work but Linux complain about not finding dl lib used  by flowvr-plugd
	#  TARGET_LINK_LIBRARIES(${target} ${FLOWVR_LIBRARY})
	  TARGET_LINK_LIBRARIES(${target} ${FLOWVR_base_LIBRARY} ${FLOWVR_ftl_LIBRARY} ${FLOWVR_mod_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES} bredala_transport bredala_datamodel)
	  INSTALL(TARGETS ${target}  DESTINATION ${MODULE_DIR})	
	ENDFOREACH(target)
endif(Boost_FOUND)

