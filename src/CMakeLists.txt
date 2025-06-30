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
        gd-ui-scaling.c gd-ui-scaling.h
        gd-application.c gd-application.h

        backends/gd-monitor.c
        backends/gd-direction.h
        backends/gd-display-config-shared.h
        backends/gd-settings.c backends/gd-settings.h
        backends/gd-edid-parse.c backends/gd-edid-private.h
        backends/gd-monitor-manager.c backends/gd-monitor-manager.h
        backends/gd-settings-private.h backends/gd-monitor-private.h
        backends/gd-backend.c backends/gd-backend.h backends/gd-backend-private.h
        backends/gd-crtc-xrandr.c backends/gd-crtc-xrandr-private.h backends/gd-crtc.c
        backends/gd-rectangle.c backends/gd-rectangle.h backends/gd-rectangle-private.h
        backends/gd-gpu-xrandr.c backends/gd-gpu-private.h backends/gd-gpu-xrandr-private.h backends/gd-gpu.c
        backends/gd-backend-x11.c backends/gd-backend-x11-private.h backends/gd-backend-x11-cm.c backends/gd-backend-x11-cm-private.h
        backends/gd-crtc-mode-info.c backends/gd-crtc-mode-info-private.h backends/gd-crtc-mode-private.h backends/gd-crtc-mode.c backends/gd-crtc-private.h
        backends/gd-logical-monitor-config.c backends/gd-logical-monitor-config-private.h backends/gd-logical-monitor-private.h backends/gd-logical-monitor.c
        backends/gd-monitor-config-manager-private.h backends/gd-monitor-config-manager.c backends/gd-monitor-config-private.h backends/gd-monitor-config-store-private.h
        backends/gd-monitor-config-store.c
        backends/gd-monitor-config-utils.h backends/gd-monitor-config.c backends/gd-monitor-config-utils.c
        backends/gd-monitor-manager-enums-private.h backends/gd-monitor-manager-kms-private.h backends/gd-monitor-manager-kms.c
        backends/gd-monitor-manager-private.h backends/gd-monitor-manager-types-private.h
        backends/gd-monitor-manager-xrandr-private.h backends/gd-monitor-manager-xrandr.c
        backends/gd-monitor-normal-private.h backends/gd-monitor-normal.c
        backends/gd-monitor-spec-private.h backends/gd-monitor-spec.c
        backends/gd-monitor-tiled-private.h backends/gd-monitor-tiled.c
        backends/gd-monitor-transform.h backends/gd-monitor-transform.c
        backends/gd-orientation-manager-private.h backends/gd-orientation-manager.c
        backends/gd-output-info-private.h backends/gd-output-info.c backends/gd-output-private.h
        backends/gd-output-xrandr-private.h backends/gd-output-xrandr.c backends/gd-output.c
        backends/gd-monitors-config-private.h backends/gd-monitors-config.c
        backends/gd-dbus-display-config.h backends/gd-dbus-display-config.c

        ${DBUS_SRC}
)
target_include_directories(graceful-DE2 PUBLIC
        ${X11_INCLUDE_DIRS}
        ${GTK_INCLUDE_DIRS}
)
target_link_libraries(graceful-DE2 PUBLIC
        -lm -lXext -lXrandr
        ${X11_LIBRARIES}
        ${GTK_LIBRARIES}
)