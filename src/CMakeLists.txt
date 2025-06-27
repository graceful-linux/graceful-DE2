execute_process(COMMAND
        gdbus-codegen --c-namespace GD --generate-c-code
        gd-session-manager-gen.dbus dbus/org.graceful.SessionManager.xml
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src)

execute_process(COMMAND
        gdbus-codegen --c-namespace GD --generate-c-code
        gd-sm-client-private-gen.dbus dbus/org.graceful.SessionManager:ClientPrivate.xml
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src)

file(GLOB DBUS_SRC *.dbus.h *.dbus.c)
add_executable(graceful-DE2 main.c
        gd-wm.c gd-wm.h
        gd-session.c gd-session.h
        gd-application.c gd-application.h
        ${DBUS_SRC}
)
target_include_directories(graceful-DE2 PUBLIC
        ${GTK_INCLUDE_DIRS}
)
target_link_libraries(graceful-DE2 PUBLIC
        -lX11
        ${GTK_LIBRARIES}
)