execute_process(COMMAND
        gdbus-codegen --c-namespace GD
        --generate-c-code gd-session-manager-gen.dbus
        dbus/org.graceful.SessionManager.xml
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src)

execute_process(COMMAND
        gdbus-codegen --c-namespace GD
        --generate-c-code gd-sm-client-private-gen.dbus
        dbus/org.graceful.SessionManager:ClientPrivate.xml
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src)

execute_process(COMMAND
        gdbus-codegen --c-namespace GD_DBus
        --generate-c-code gd-dbus-x11.dbus
        --interface-prefix org.gnome.Mutter
        dbus/org.graceful.Mutter.X11.xml
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src)

execute_process(COMMAND
        gdbus-codegen --c-namespace GD
        --generate-c-code gd-file-manager-gen.dbus
        dbus/org.freedesktop.FileManager1.xml
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src)

execute_process(COMMAND
        gdbus-codegen --c-namespace GD
        --generate-c-code gd-nautilus2-gen.dbus
        dbus/org.gnome.Nautilus.FileOperations2.xml
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src)

execute_process(COMMAND
        gdbus-codegen --c-namespace GD
        --generate-c-code gd-sn-watcher-vo-gen.dbus
        dbus/org.kde.StatusNotifierWatcher.xml
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src)

execute_process(COMMAND
        gdbus-codegen --c-namespace GD
        --generate-c-code gd-fd-notifications-gen.dbus
        dbus/org.freedesktop.Notifications.xml
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src)

execute_process(COMMAND
        gdbus-codegen --c-namespace GD
        --generate-c-code gd-shell-introspect-gen.dbus
        dbus/org.graceful.DE2.Introspect.xml
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src)

execute_process(COMMAND
        gdbus-codegen --c-namespace GD
        --generate-c-code gd-shell-gen.dbus
        dbus/org.graceful.DE2.xml
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src)

file(GLOB DBUS_SRC *.dbus.h *.dbus.c)
add_executable(graceful-DE2 main.c
        gd-wm.c gd-wm.h
        gd-session.c gd-session.h
        gd-ui-scaling.c gd-ui-scaling.h
        gd-application.c gd-application.h
        gd-confirm-display-change-dialog.c gd-confirm-display-change-dialog.h

        backends/gd-monitor.c
        backends/gd-settings.c
        backends/gd-settings.h
        backends/gd-direction.h
        backends/gd-edid-parse.c
        backends/gd-edid-private.h
        backends/gd-display-config-shared.h
        backends/gd-monitor-config-store-private.h
        backends/gd-monitor-config-manager-private.h
        backends/gd-gpu-xrandr.c backends/gd-gpu-private.h
        backends/gd-gpu-xrandr-private.h backends/gd-gpu.c
        backends/gd-backend-x11.c backends/gd-backend-x11-private.h
        backends/gd-monitor-manager.c backends/gd-monitor-manager.h
        backends/gd-settings-private.h backends/gd-monitor-private.h
        backends/gd-monitor-spec-private.h backends/gd-monitor-spec.c
        backends/gd-monitor-tiled-private.h backends/gd-monitor-tiled.c
        backends/gd-monitor-transform.h backends/gd-monitor-transform.c
        backends/gd-monitor-config-store.c backends/gd-monitor-config.c
        backends/gd-backend-x11-cm.c backends/gd-backend-x11-cm-private.h
        backends/gd-monitor-normal-private.h backends/gd-monitor-normal.c
        backends/gd-crtc-mode-info.c backends/gd-crtc-mode-info-private.h
        backends/gd-monitors-config-private.h backends/gd-monitors-config.c
        backends/gd-dbus-display-config.h backends/gd-dbus-display-config.c
        backends/gd-logical-monitor-private.h backends/gd-logical-monitor.c
        backends/gd-monitor-config-utils.h backends/gd-monitor-config-utils.c
        backends/gd-monitor-config-manager.c backends/gd-monitor-config-private.h
        backends/gd-backend.c backends/gd-backend.h backends/gd-backend-private.h
        backends/gd-orientation-manager-private.h backends/gd-orientation-manager.c
        backends/gd-crtc-xrandr.c backends/gd-crtc-xrandr-private.h backends/gd-crtc.c
        backends/gd-rectangle.c backends/gd-rectangle.h backends/gd-rectangle-private.h
        backends/gd-logical-monitor-config.c backends/gd-logical-monitor-config-private.h
        backends/gd-monitor-manager-private.h backends/gd-monitor-manager-types-private.h
        backends/gd-monitor-manager-xrandr-private.h backends/gd-monitor-manager-xrandr.c
        backends/gd-crtc-mode-private.h backends/gd-crtc-mode.c backends/gd-crtc-private.h
        backends/gd-output-xrandr-private.h backends/gd-output-xrandr.c backends/gd-output.c
        backends/gd-monitor-manager-enums-private.h backends/gd-monitor-manager-kms-private.h backends/gd-monitor-manager-kms.c
        backends/gd-output-info-private.h backends/gd-output-info.c backends/gd-output-private.h

        plugins/common/gd-bg.h plugins/common/gd-bg.c
        plugins/common/gd-keybindings.h plugins/common/gd-keybindings.c
        plugins/common/gd-popup-window.h plugins/common/gd-popup-window.c
        plugins/common/gd-common-enum-types.h plugins/common/gd-common-enum-types.c

        plugins/desktop/gd-desktop-enums.h
        plugins/desktop/gd-icon.h plugins/desktop/gd-icon.c
        plugins/desktop/gd-utils.h plugins/desktop/gd-utils.c
        plugins/desktop/gd-desktop.h plugins/desktop/gd-desktop.c
        plugins/desktop/gd-icon-view.h plugins/desktop/gd-icon-view.c
        plugins/desktop/gd-home-icon.h plugins/desktop/gd-home-icon.c
        plugins/desktop/gd-trash-icon.h plugins/desktop/gd-trash-icon.c
        plugins/desktop/gd-background.h plugins/desktop/gd-background.c
        plugins/desktop/gd-dummy-icon.h plugins/desktop/gd-dummy-icon.c
        plugins/desktop/gd-monitor-view.h plugins/desktop/gd-monitor-view.c
        plugins/desktop/gd-rename-popover.h plugins/desktop/gd-rename-popover.c
        plugins/desktop/gd-desktop-window.h plugins/desktop/gd-desktop-window.c
        plugins/desktop/gd-workarea-watcher.h plugins/desktop/gd-workarea-watcher.c
        plugins/desktop/gd-thumbnail-factory.h plugins/desktop/gd-thumbnail-factory.c
        plugins/desktop/gd-desktop-enum-types.h plugins/desktop/gd-desktop-enum-types.c
        plugins/desktop/gd-create-folder-dialog.h plugins/desktop/gd-create-folder-dialog.c

        plugins/notifications/nd-queue.h plugins/notifications/nd-queue.c
        plugins/notifications/nd-stack.h plugins/notifications/nd-stack.c
        plugins/notifications/gd-bubble.h plugins/notifications/gd-bubble.c
        plugins/notifications/nd-daemon.h plugins/notifications/nd-daemon.c
        plugins/notifications/nd-notification.h plugins/notifications/nd-notification.c
        plugins/notifications/gd-notifications.h plugins/notifications/gd-notifications.c
        plugins/notifications/nd-notification-box.h plugins/notifications/nd-notification-box.c

        plugins/shell/osd.h plugins/shell/osd.c
        plugins/shell/shell.h plugins/shell/shell.c
        plugins/shell/gd-osd-window.h plugins/shell/gd-osd-window.c
        plugins/shell/monitor-labeler.h plugins/shell/monitor-labeler.c
        plugins/shell/gd-label-window.h plugins/shell/gd-label-window.c
        plugins/shell/gd-shell-introspect.h plugins/shell/gd-shell-introspect.c

        plugins/background/gd-root-background.h plugins/background/gd-root-background.c

        plugins/status-notify-watcher/gd-sn-watcher-v0.h plugins/status-notify-watcher/gd-sn-watcher-v0.c
        plugins/status-notify-watcher/gd-status-notifier-watcher.h plugins/status-notify-watcher/gd-status-notifier-watcher.c

        ${DBUS_SRC}
)
target_include_directories(graceful-DE2 PUBLIC
        ${X11_INCLUDE_DIRS}
        ${GTK_INCLUDE_DIRS}
        ${GNOME_INCLUDE_DIRS}
        ${SYSTEMD_INCLUDE_DIRS}
)
target_link_libraries(graceful-DE2 PUBLIC
        -lm -lXext -lXrandr
        ${X11_LIBRARIES}
        ${GTK_LIBRARIES}
        ${GNOME_LIBRARIES}
        ${SYSTEMD_LIBRARIES}
)

install(TARGETS graceful-DE2 DESTINATION libexec/)
