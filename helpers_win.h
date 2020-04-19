#include <windows.h>

void GerWorkingDirectory(char* buffer, DWORD length) {
  DWORD path_length;
  path_length = GetModuleFileNameA(0, buffer, length);
  // strip exe name
  unsigned int i;
  for (i = path_length - 1; i > 0; i--) {
    if (buffer[i] == '\\')
      break;
    else
      buffer[i] = 0;
  }
  buffer[i] = '\0';
}