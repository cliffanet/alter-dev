/*
    Jump worker
*/

#ifndef _jump_H
#define _jump_H

#define PINBMP  5

// Шаг отображения высоты
#define ALT_STEP                5
// Порог перескока к следующему шагу
#define ALT_STEP_ROUND          3

// Интервал обнуления высоты (ms)
#define ALT_AUTOGND_INTERVAL    600000

bool jumpStart();
bool jumpStop();

#endif // _jump_H
