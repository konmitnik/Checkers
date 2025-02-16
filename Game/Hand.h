#pragma once
#include <tuple>

#include "../Models/Move.h"
#include "../Models/Response.h"
#include "Board.h"

// methods for hands
class Hand
{
public:
    Hand(Board* board) : board(board) // конструктор
    {
    }
    tuple<Response, POS_T, POS_T> get_cell() const // функция получения координат клетки от игрока
    {
        SDL_Event windowEvent;
        Response resp = Response::OK;
        int x = -1, y = -1;
        int xc = -1, yc = -1;
        while (true)
        {
            if (SDL_PollEvent(&windowEvent))
            {
                switch (windowEvent.type) // проверка типа ввода игрока
                {
                case SDL_QUIT: // если выход, то возвращаем соотетствующий ответ
                    resp = Response::QUIT;
                    break;
                case SDL_MOUSEBUTTONDOWN: // если нажата кнопка мышь
                    x = windowEvent.motion.x; // получение координат, где нажали на кнопку мыши
                    y = windowEvent.motion.y;
                    xc = int(y / (board->H / 10) - 1); // расчет координат клетки
                    yc = int(x / (board->W / 10) - 1);
                    if (xc == -1 && yc == -1 && board->history_mtx.size() > 1) // если нажата кнопка "Назад"
                    {
                        resp = Response::BACK;
                    }
                    else if (xc == -1 && yc == 8) // если нажата кнопка "Перезапуск"
                    {
                        resp = Response::REPLAY;
                    }
                    else if (xc >= 0 && xc < 8 && yc >= 0 && yc < 8) // если была нажата клетка
                    {
                        resp = Response::CELL;
                    }
                    else // иначе сброс координат клетки
                    {
                        xc = -1;
                        yc = -1;
                    }
                    break;
                case SDL_WINDOWEVENT: // если событие связано с изменением размера экрана, то применяем новые настройки
                    if (windowEvent.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                    {
                        board->reset_window_size();
                        break;
                    }
                }
                if (resp != Response::OK) // если получили ответ, то выходим из цикла
                    break;
            }
        }
        return { resp, xc, yc }; // возвращаем ответ
    }

    Response wait() const // ожидание ввода игрока после игры
    {
        SDL_Event windowEvent;
        Response resp = Response::OK;
        while (true)
        {
            if (SDL_PollEvent(&windowEvent))
            {
                switch (windowEvent.type) // проверка типа введеного значения
                {
                case SDL_QUIT: // если закрытие окна
                    resp = Response::QUIT;
                    break;
                case SDL_WINDOWEVENT_SIZE_CHANGED: // если изменение размера окна
                    board->reset_window_size();
                    break;
                case SDL_MOUSEBUTTONDOWN: { //если нажата кнопка мыши
                    int x = windowEvent.motion.x; // определение координат, где нажала мышь
                    int y = windowEvent.motion.y;
                    int xc = int(y / (board->H / 10) - 1); // определение координат клетки
                    int yc = int(x / (board->W / 10) - 1);
                    if (xc == -1 && yc == 8) // если нажата кнопка "перезапуск"
                        resp = Response::REPLAY;
                }
                                        break;
                }
                if (resp != Response::OK) // если получен ответ, выходим из цикла
                    break;
            }
        }
        return resp;
    }

private:
    Board* board; // указатель на объект игрового поля
};
