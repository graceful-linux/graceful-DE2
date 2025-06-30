//
// Created by dingjing on 25-6-28.
//

#ifndef graceful_DE2_GD_MONITOR_MANAGER_TYPES_PRIVATE_H
#define graceful_DE2_GD_MONITOR_MANAGER_TYPES_PRIVATE_H
#include "macros/macros.h"

C_BEGIN_EXTERN_C

typedef enum _GDMonitorsConfigFlag GDMonitorsConfigFlag;

typedef struct _GDDBusDisplayConfig GDDBusDisplayConfig;

typedef struct _GDLogicalMonitor GDLogicalMonitor;

typedef struct _GDMonitor GDMonitor;
typedef struct _GDMonitorSpec GDMonitorSpec;
typedef struct _GDMonitorMode GDMonitorMode;
typedef struct _GDMonitorsConfig GDMonitorsConfig;
typedef struct _GDMonitorsConfigKey GDMonitorsConfigKey;
typedef struct _GDMonitorConfigStore GDMonitorConfigStore;
typedef struct _GDMonitorConfigManager GDMonitorConfigManager;

typedef struct _GDGpu GDGpu;

typedef struct _GDCrtc GDCrtc;
typedef struct _GDCrtcMode GDCrtcMode;

typedef struct _GDOutput GDOutput;
typedef struct _GDOutputCtm GDOutputCtm;
typedef struct _GDOutputInfo GDOutputInfo;


C_END_EXTERN_C

#endif // graceful_DE2_GD_MONITOR_MANAGER_TYPES_PRIVATE_H
