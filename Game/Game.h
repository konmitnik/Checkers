#pragma once
#include <chrono>
#include <thread>

#include "../Models/Project_path.h"
#include "Board.h"
#include "Config.h"
#include "Hand.h"
#include "Logic.h"

class Game
{
  public:
    Game() : board(config("WindowSize", "Width"), config("WindowSize", "Hight")), hand(&board), logic(&board, &config)
    {
        ofstream fout(project_path + "log.txt", ios_base::trunc);
        fout.close();
    }

    // to start checkers
    int play()
    {
        auto start = chrono::steady_clock::now(); // отсчет времени игры
        if (is_replay) // обработка перезапуска игры
        {
            logic = Logic(&board, &config);
            config.reload();
            board.redraw();
        }
        else
        {
            board.start_draw(); // отрисовка новой игры
        }
        is_replay = false;

        int turn_num = -1; // счетчик ходов
        bool is_quit = false; // проверка на то, хочет ли игрок выйти
        const int Max_turns = config("Game", "MaxNumTurns"); // получение максимального количества ходов из конфига
        while (++turn_num < Max_turns) // запуск цикла игры, пока количество сделаных ходов меньше максимального
        {
            beat_series = 0;
            logic.find_turns(turn_num % 2); // просчет возможных ходов для игрока и бота
            if (logic.turns.empty()) // если возможных ходов нет - игра окончена
                break;
            logic.Max_depth = config("Bot", string((turn_num % 2) ? "Black" : "White") + string("BotLevel")); // максимальная глубина бота на основе его уровня, прописанного в конфиге
            if (!config("Bot", string("Is") + string((turn_num % 2) ? "Black" : "White") + string("Bot"))) // если текущий ход игрока
            {
                auto resp = player_turn(turn_num % 2); // считывание ввода игрока
                if (resp == Response::QUIT) // обработка выхода из игры
                {
                    is_quit = true;
                    break;
                }
                else if (resp == Response::REPLAY) // обработка перезапуска игры
                {
                    is_replay = true;
                    break;
                }
                else if (resp == Response::BACK) // обработка отката игры на 1 ход назад
                {
                    if (config("Bot", string("Is") + string((1 - turn_num % 2) ? "Black" : "White") + string("Bot")) &&
                        !beat_series && board.history_mtx.size() > 2) // откат хода возможен, если ход игрока
                    {
                        board.rollback();
                        --turn_num;
                    }
                    if (!beat_series) // если не было серии ходов, то откат номера хода
                        --turn_num;

                    board.rollback(); // откат хода
                    --turn_num;
                    beat_series = 0;
                }
            }
            else // ткекущий ход бота
                bot_turn(turn_num % 2); // выбполнение хода за бота
        }
        auto end = chrono::steady_clock::now(); // замер времени конца игры
        ofstream fout(project_path + "log.txt", ios_base::app); // открытие файла для записи информации об игре
        fout << "Game time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n"; // запись времени игры в файл
        fout.close(); // закрытие файла

        if (is_replay) // если перезапуск игры, то рекурсивный вызов функции play
            return play();
        if (is_quit) // если выход, то завершение работы программы
            return 0;
        int res = 2; // определение результата игры, победа белых по умолчанию
        if (turn_num == Max_turns) // ничья при достижении максимума ходов
        {
            res = 0;
        }
        else if (turn_num % 2) // победа черных, если был их ход
        {
            res = 1;
        }
        board.show_final(res); // вывод результата игры
        auto resp = hand.wait(); // ожидание ввода от игрока
        if (resp == Response::REPLAY) // рекурсивный запуск функции play, если игрок решает перезапустить игру
        {
            is_replay = true;
            return play();
        }
        return res;
    }

  private:
    void bot_turn(const bool color) // ход бота
    {
        auto start = chrono::steady_clock::now(); // начало отсчета времени хода

        auto delay_ms = config("Bot", "BotDelayMS"); 
        // new thread for equal delay for each turn
        thread th(SDL_Delay, delay_ms); // новый поток для задержки хода
        auto turns = logic.find_best_turns(color); // поиск лучшего хода для цвета бота
        th.join(); // ожидание выполнения потока
        bool is_first = true;
        // making moves
        for (auto turn : turns) // ход
        {
            if (!is_first) // если ход не первый, то задержка
            {
                SDL_Delay(delay_ms);
            }
            is_first = false;
            beat_series += (turn.xb != -1); // если серия ходов (при "съедании" шашки), то увеличивает счетчик серии ходов
            board.move_piece(turn, beat_series); // перемещение шашки
        }

        auto end = chrono::steady_clock::now(); // конец отсчета времени хода
        ofstream fout(project_path + "log.txt", ios_base::app);
        fout << "Bot turn time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n"; // запись времени хода бота
        fout.close();
    }

    Response player_turn(const bool color) // аргументом функции указывается цвет, за который играет игрок
    {
        // return 1 if quit
        vector<pair<POS_T, POS_T>> cells;
        for (auto turn : logic.turns)
        {
            cells.emplace_back(turn.x, turn.y);// добавление в вектор клеток возможных ходов
        }
        board.highlight_cells(cells); // подсвечивает зеленым клетки, куда можно пойти
        move_pos pos = {-1, -1, -1, -1};
        POS_T x = -1, y = -1;
        // trying to make first move
        while (true)
        {
            auto resp = hand.get_cell(); // получение клетки, на которую нажал игрок
            if (get<0>(resp) != Response::CELL)
                return get<0>(resp); // если не клетка возвращает ответ
            pair<POS_T, POS_T> cell{get<1>(resp), get<2>(resp)};

            bool is_correct = false;
            for (auto turn : logic.turns) // проверка возможности совершить выбранный ход
            {
                if (turn.x == cell.first && turn.y == cell.second)
                {
                    is_correct = true;
                    break;
                }
                if (turn == move_pos{x, y, cell.first, cell.second})
                {
                    pos = turn;
                    break;
                }
            }
            if (pos.x != -1) // если ход возможен, то выходим из цикла
                break;
            if (!is_correct) // если ход не возможен, то сбрасываем координаты
            {
                if (x != -1)
                {
                    board.clear_active();
                    board.clear_highlight();
                    board.highlight_cells(cells);
                }
                x = -1;
                y = -1;
                continue;
            }
            x = cell.first;
            y = cell.second;
            board.clear_highlight(); // удаление подсветки доступных ходов
            board.set_active(x, y); // делаем активной клетку с выбранными координатами
            vector<pair<POS_T, POS_T>> cells2; // собираем клетки для хода
            for (auto turn : logic.turns)
            {
                if (turn.x == x && turn.y == y)
                {
                    cells2.emplace_back(turn.x2, turn.y2);
                }
            }
            board.highlight_cells(cells2); // подсвечиваем клетки для хода
        }
        board.clear_highlight(); // удаление подсветки доступных ходов
        board.clear_active();
        board.move_piece(pos, pos.xb != -1); // перемещение шашки
        if (pos.xb == -1) // если не было "съедено" вражеских фигур, то возвращаем ОК
            return Response::OK;
        // continue beating while can
        beat_series = 1; // иначе начинаем серию ходов, пока можем
        while (true)
        {
            logic.find_turns(pos.x2, pos.y2); // находим возможные ходы
            if (!logic.have_beats) // если "есть" больше некого, то выходим из цикла
                break;

            vector<pair<POS_T, POS_T>> cells;
            for (auto turn : logic.turns) // добавляем возможные ходы в вектор
            {
                cells.emplace_back(turn.x2, turn.y2);
            }
            board.highlight_cells(cells); // подсвечиваем возможные ходы
            board.set_active(pos.x2, pos.y2);
            // trying to make move
            while (true)
            {
                auto resp = hand.get_cell(); // получение ввода от игрока
                if (get<0>(resp) != Response::CELL) // если ввод - не клетка, то выходим
                    return get<0>(resp);
                pair<POS_T, POS_T> cell{get<1>(resp), get<2>(resp)}; // получение координат клетки от игрока

                bool is_correct = false;
                for (auto turn : logic.turns) // проверка на возможность совершения хода
                {
                    if (turn.x2 == cell.first && turn.y2 == cell.second)
                    {
                        is_correct = true;
                        pos = turn;
                        break;
                    }
                }
                if (!is_correct) // если выбранная клетка не подходит для хода, то повторить
                    continue;

                board.clear_highlight(); // удаление подсветки
                board.clear_active();
                beat_series += 1; // увеличение счетчика серии
                board.move_piece(pos, beat_series); // перемещение шашки
                break;
            }
        }

        return Response::OK; // возвращаем ОК
    }

  private:
    Config config;
    Board board;
    Hand hand;
    Logic logic;
    int beat_series;
    bool is_replay = false;
};
