#ifndef CAMTASK_H
#define CAMTASK_H

/* Cam task: capture loop and task lifecycle.
   Implemented in camprocessor.cpp so builds that do not compile a separate
   camtask.cpp (e.g. USBFlasher sketch copy) still link. */

void CamTask_Start(void);
void CamTask_Delete(void);

#endif
