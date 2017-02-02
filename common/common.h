#pragma once

#define APPLICATION_RUNNING_MUTEX           L"{6CB8A2C1-B4CA-489f-A3C4-859F0DA8F9B2}"
#define APPLICATION_MAINWND_CLASS_NAME			L"{2327497B-909A-4d05-BF81-9B785A8A025E}"
#define MESSAGE_QUEUE_FILES                 L"{340DBFC2-B5D2-4318-82C2-4CF7AB62C51B}"
#define QUEUE_MEMORY_FILE_NAME              L"{B53957F9-EDDB-4057-9282-72CE931EA3F5}"

#define APPLICATION_REGISTRY_KEY            L"Software\\Haephrati"
#define REG_EXECUTABLE                      L"Executable"

typedef struct
{
  size_t cb;                                // count of bytes
  int menuCommand;                          // context menu command index starting from 0
  size_t listChars;                         // size of the list that follows the structure in chars
} MEMORY_MAPPED_INFO;