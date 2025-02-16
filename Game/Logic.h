#pragma once
#include <random>
#include <vector>

#include "../Models/Move.h"
#include "Board.h"
#include "Config.h"

const int INF = 1e9;

class Logic
{
  public:
    Logic(Board *board, Config *config) : board(board), config(config)
    {
        rand_eng = std::default_random_engine (
            !((*config)("Bot", "NoRandom")) ? unsigned(time(0)) : 0);
        scoring_mode = (*config)("Bot", "BotScoringType");
        optimization = (*config)("Bot", "Optimization");
    }

    vector<move_pos> find_best_turns(const bool color) // метод для нахождения лучших ходов для указанного цвета
    {
        next_move.clear(); // очистка информации о старых "следующих ходах" и "состояниях"
        next_best_state.clear();

        find_first_best_turn(board->get_board(), color, -1, -1, 0); // поиск первого лучшего хода

        vector<move_pos> best_moves; // вектор лучших ходов
        int state = 0; // состояние, с которого начинается поиск
        
        while (state != -1 && next_move[state].x != -1) { // пока есть доступные ходы
            best_moves.push_back(next_move[state]); // добавление лучшего хода в вектр
            state = next_best_state[state]; // переход к слежуающему лучшему ходу
        }

        return best_moves; // в результате отдается вектор с лучшими ходами
    }

private:
    vector<vector<POS_T>> make_turn(vector<vector<POS_T>> mtx, move_pos turn) const // функция выполнения хода
    {
        if (turn.xb != -1)
            mtx[turn.xb][turn.yb] = 0; // удаление шашки
        if ((mtx[turn.x][turn.y] == 1 && turn.x2 == 0) || (mtx[turn.x][turn.y] == 2 && turn.x2 == 7))
            mtx[turn.x][turn.y] += 2; // превращение шашки в ферзя
        mtx[turn.x2][turn.y2] = mtx[turn.x][turn.y]; // перемещение
        mtx[turn.x][turn.y] = 0; // "обнуление" координат
        return mtx;
    }

    double calc_score(const vector<vector<POS_T>> &mtx, const bool first_bot_color) const // оценка состояния доски
    {
        // color - who is max player
        double w = 0, wq = 0, b = 0, bq = 0;
        for (POS_T i = 0; i < 8; ++i) // определение каких шашек больше
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                w += (mtx[i][j] == 1); // белые шашки
                wq += (mtx[i][j] == 3); // белые дамки
                b += (mtx[i][j] == 2); // черные шашки
                bq += (mtx[i][j] == 4); // черные дамки
                if (scoring_mode == "NumberAndPotential") // определение потенциала
                {
                    w += 0.05 * (mtx[i][j] == 1) * (7 - i); // белых
                    b += 0.05 * (mtx[i][j] == 2) * (i); // черных
                }
            }
        }
        if (!first_bot_color)
        {
            swap(b, w);
            swap(bq, wq);
        }
        if (w + wq == 0)
            return INF;
        if (b + bq == 0)
            return 0;
        int q_coef = 4;
        if (scoring_mode == "NumberAndPotential")
        {
            q_coef = 5;
        }
        return (b + bq * q_coef) / (w + wq * q_coef); // отношение суммарных значений шашек черных и белых
    }

    double find_first_best_turn(vector<vector<POS_T>> mtx, const bool color, const POS_T x, const POS_T y, size_t state,
                                double alpha = -1) // рекурсивный метод для поиска первого лучшего хода в заданной позиции
    {
        next_move.emplace_back(-1, -1, -1, -1); // инициализация нового хода 
        next_best_state.push_back(-1);
        
        if (state != 0) { // поиск возможных ходов
            find_turns(x, y, mtx);
        }
        
        auto now_turns = turns; // возможные лучшие ходы в настоящий момент
        auto can_beats = have_beats; // можно ли "съесть" шашку

        if (!can_beats && state != 0) { // если нет возможности съесть, то ищем лучшие ходы для другого цвета
            return find_best_turns_rec(mtx, 1 - color, 0, alpha);
        }

        double best_score = -1; // инициализация лучшего счёта
        
        for (auto turn : now_turns) {
            size_t new_state = next_move.size(); // следующеe состояниe
            double current_score;
            if (can_beats) { // если можно съесть шашку
                current_score = find_first_best_turn(make_turn(mtx, turn), color, turn.x2, turn.y2, new_state, best_score); // рекурсивный вызов поиска следующего лучшего хода
            } else {
                current_score = find_best_turns_rec(make_turn(mtx, turn), 1 - color, 0, best_score); // иначе ищем лучшие ходы для другого цвета
            }

            if (current_score > best_score) {
                best_score = current_score; // обновление лучшего счёта
                next_move[state] = turn; // сохранение текущего лушего хода состояния
                next_best_state[state] = (can_beats ? new_state : -1); // определение следующего состояния
            }
        }
        return best_score;
    }

    double find_best_turns_rec(vector<vector<POS_T>> mtx, const bool color, const size_t depth, double alpha = -1,
                               double beta = INF + 1, const POS_T x = -1, const POS_T y = -1) // рекурсивный метод для поиска лучших ходов с учётом текущего состояния и глубины поиска
    {
            if (depth == Max_depth) { // если достигнут лимит по глубине
                return calc_score(mtx, (depth % 1 == color)); // оценка позиции для текузего хода
            }

            if (x != -1) { // если есть координаты, то ищем лучший ход для шашки данного цвета по этим координатам
                find_turns(x, y, mtx);
            } else { // иначе для всех текущего цвета
                find_turns(color, mtx);
            }
            
            auto now_turns = turns; // текущие лучшие ходы
            auto can_beats = have_beats; // есть ли возможность "съесть" шашку
            
            if (!can_beats && x != -1) { // если нет возможности съесть шашку, то рекурсивно ищем лучшие ходы другого цвета
                return find_best_turns_rec(mtx, 1 - color, +1, alpha, beta);
            }

            if (turns.empty()) { // если ходов нет, то возвращается 0 для своего цвета, INF для противника
                return (depth % 2 ? 0 : INF);
            }

            double min_score = INF + 1; // минимальный счет
            double max_score = -1; // максимальный счет
            
            for (auto turn : now_turns) {
                double score;
                
                if (can_beats) { // если можно съесть, то ищем дальнейшие лучшие ходы
                    score = find_best_turns_rec(make_turn(mtx, turn), color, depth, alpha, beta, turn.x2, turn.y2);
                } else { // иначе ищем лучшие ходы другого цвета
                    score = find_best_turns_rec(make_turn(mtx, turn), 1 - color, depth + 1, alpha, beta);
                }

                min_score = min(min_score, score); // обновление минимального цвета
                max_score = max(max_score, score); // обновление максимального цвета
                
                if (depth % 2) { // alpha-beta отсечение
                    alpha = max(alpha, max_score); // если игрок максимизирует счет
                } else {
                    beta = min(beta, min_score); // если игрок минимизирует счет
                }
                
                if (optimization != "O0" && alpha > beta) { // если alpha > beta, то выход из цикла
                    break;
                }
                
                if (optimization == "O2" && alpha == beta) { // проверка оптимизации
                    return (depth % 2 ? max_score + 1 : min_score - 1);
                }
            }

            return (depth % 2 ? max_score : min_score); // возвращение лучшего счета в зависимости от цвета
    }

public:
    void find_turns(const bool color) // публичная перегрузка поиска хода на основе цвета
    {
        find_turns(color, board->get_board());
    }

    void find_turns(const POS_T x, const POS_T y) // публичная перегрузка поиска хода на основе координат
    {
        find_turns(x, y, board->get_board());
    }

private:
    void find_turns(const bool color, const vector<vector<POS_T>> &mtx) // приватная перегрузка поиска хода на основе цвета и состояния доски 
    {
        vector<move_pos> res_turns;
        bool have_beats_before = false;
        for (POS_T i = 0; i < 8; ++i) // поиск хода по всем клеткам доски
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                if (mtx[i][j] && mtx[i][j] % 2 != color)
                {
                    find_turns(i, j, mtx);
                    if (have_beats && !have_beats_before) // проверка возможности "съесть" и что не "ели" раньше
                    {
                        have_beats_before = true;
                        res_turns.clear();
                    }
                    if ((have_beats_before && have_beats) || !have_beats_before) // запись возможных ходов
                    {
                        res_turns.insert(res_turns.end(), turns.begin(), turns.end());
                    }
                }
            }
        }
        turns = res_turns;
        shuffle(turns.begin(), turns.end(), rand_eng);
        have_beats = have_beats_before;
    }

    void find_turns(const POS_T x, const POS_T y, const vector<vector<POS_T>> &mtx) // приватная перегрузка функции для поиска хода по координатам клетки и состоянию доски
    {
        turns.clear(); // очистка старой информации о возможных ходах
        have_beats = false;
        POS_T type = mtx[x][y];
        // check beats
        switch (type) // проверка возможности "съесть"
        {
        case 1:
        case 2:
            // check pieces
            for (POS_T i = x - 2; i <= x + 2; i += 4) // проверка шашек
            {
                for (POS_T j = y - 2; j <= y + 2; j += 4)
                {
                    if (i < 0 || i > 7 || j < 0 || j > 7)
                        continue;
                    POS_T xb = (x + i) / 2, yb = (y + j) / 2;
                    if (mtx[i][j] || !mtx[xb][yb] || mtx[xb][yb] % 2 == type % 2)
                        continue;
                    turns.emplace_back(x, y, i, j, xb, yb);
                }
            }
            break;
        default:
            // check queens
            for (POS_T i = -1; i <= 1; i += 2) // проверка дамок
            {
                for (POS_T j = -1; j <= 1; j += 2)
                {
                    POS_T xb = -1, yb = -1;
                    for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j)
                    {
                        if (mtx[i2][j2])
                        {
                            if (mtx[i2][j2] % 2 == type % 2 || (mtx[i2][j2] % 2 != type % 2 && xb != -1))
                            {
                                break;
                            }
                            xb = i2;
                            yb = j2;
                        }
                        if (xb != -1 && xb != i2)
                        {
                            turns.emplace_back(x, y, i2, j2, xb, yb);
                        }
                    }
                }
            }
            break;
        }
        // check other turns
        if (!turns.empty()) // проверка остальных ходов
        {
            have_beats = true;
            return;
        }
        switch (type)
        {
        case 1:
        case 2:
            // check pieces
            {
                POS_T i = ((type % 2) ? x - 1 : x + 1); // проверка шашек
                for (POS_T j = y - 1; j <= y + 1; j += 2)
                {
                    if (i < 0 || i > 7 || j < 0 || j > 7 || mtx[i][j])
                        continue;
                    turns.emplace_back(x, y, i, j);
                }
                break;
            }
        default:
            // check queens
            for (POS_T i = -1; i <= 1; i += 2) // проверка дамок
            {
                for (POS_T j = -1; j <= 1; j += 2)
                {
                    for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j)
                    {
                        if (mtx[i2][j2])
                            break;
                        turns.emplace_back(x, y, i2, j2);
                    }
                }
            }
            break;
        }
    }

  public:
    vector<move_pos> turns; // список возможных ходов
    bool have_beats; // были ли "съедены" шашки
    int Max_depth; // максимальная глубина бота

  private:
    default_random_engine rand_eng; // движок рандомных значений для генерации псевдо-случайных чисел
    string scoring_mode; // метод подсчета
    string optimization; // оптимизация хода
    vector<move_pos> next_move; // следующий ход
    vector<int> next_best_state; // состояние следующего лучшего хода
    Board *board; // доска
    Config *config; // конфиг игры
};
