/*
    Jump worker
*/

#ifndef _jump_H
#define _jump_H

#define PINBMP  5

// Шаг отображения высоты
#define ALT_STEP            5
// Порог перескока к следующему шагу
#define ALT_STEP_ROUND      3

bool jumpStart();
bool jumpStop();

#endif // _jump_H
