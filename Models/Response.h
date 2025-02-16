#pragma once

enum class Response
{
    OK, // принятие действия
    BACK, // вернуться на ход назад
    REPLAY, // перезапуск игры
    QUIT, // выход из игры
    CELL // нажатие на клетку на игровой доске
};
