#include <iostream>
#include <mutex>
#include <condition_variable>
#include <semaphore.h>
#include <vector>
#include <thread>
#include <random>

#pragma ide diagnostic ignored "EndlessLoop"

bool letsCheck = false; //Флаг проверки
std::vector<bool> isReady;
std::vector<bool> isCorrect;
std::condition_variable checkWork; //Условная переменная регулировки проверки работ
std::mutex checkMtx; //Мьютекс для проверки работ

int countOfProgramms = 0; //Количество созданных программ

const int minCodeTime = 100; //Минимальное время написания программы
const int maxCodeTime = 500; //Максимальное время написания программы
const int minCheckTime = 100; //Минимальное время проверки программы
const int maxCheckTime = 300; //Максимальное время проверки программы

/**
 * Находит индекс непроверенной работы и в случае
 * если его нет выводит -1
 * @param myIndex
 * @return
 */
int findNotCheckWork(int myIndex) {
    for (int i = 0; i < 3; ++i) {
        if (isReady[i] && !isCorrect[i] && i != myIndex)
            return i;
    }
    return -1;
}

/**
 * Реализует поток программиста
 * @param index индекс программиста
 */
void programmer(int index) {
    srand(time(0) * index); //Устанавливаем рандом
    std::unique_lock<std::mutex> checkLock(checkMtx); //Локер для условной переменной
    checkLock.unlock(); //Разлочиваем созданный локер
    while (true) {
        std::printf("Prog %d: \tI am ready to do new program\n", index);
        //Устанавливаем флаги готовности и корректности написанной программы
        isReady[index] = false;
        isCorrect[index] = false;

        while (!isCorrect[index]) { //Пока программа не корректна пытаемся ее сделать
            std::printf("Prog %d: \tI start do my work\n", index);
            //Имитируем работу программиста
            std::this_thread::sleep_for(std::chrono::milliseconds(std::rand() %
                                (maxCodeTime - minCodeTime) + minCodeTime));
            std::printf("Prog %d: \tI finished my work\n", index);

            isReady[index] = true; //Устанавливаем флаг готовности работы
            checkWork.notify_one(); //Говорим проверяющему, что появилась работа на проверку
            letsCheck = true; //Устанавливаем флаг проверки в true, чтобы проверящий вышел из цикла
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); //Даем проверяющему время для выхода

            while (isReady[index] && !isCorrect[index]) { //Цикл проверки (Проверяем пока не пришли резы проги)
                std::printf("Prog %d: \tI start check works\n", index);
                //Цикл устраняющий случайное пробуждение потока
                while (!letsCheck && !isCorrect[index] && isReady[index])
                    checkWork.wait(checkLock);
                letsCheck = false; //Устанавливаем флаг проверки в false

                int checkedIndex = findNotCheckWork(index); //Находим индекс непроверенной работы
                //Начниаем проверку если наша работа готова, но не проверена
                if (checkedIndex != -1 && !isCorrect[index] && isReady[index]) {
                    std::printf("Prog %d: \tI check work from Prog %d\n", index, checkedIndex);
                    std::this_thread::sleep_for(std::chrono::milliseconds(std::rand() %
                                                                          (maxCheckTime - minCheckTime) + minCodeTime));
                    bool correct = std::rand() % 2 == 0; //Выносим вердикт
                    if (!correct) {
                        isReady[checkedIndex] = false;
                        std::printf("Prog %d: \tProg %d has incorrect code\n", index, checkedIndex);
                        std::printf("Prog %d: \tStart to redoing his work\n", checkedIndex);
                    } else {
                        std::printf("Prog %d: \tProg %d has good code\n", index, checkedIndex);
                    }
                    isCorrect[checkedIndex] = correct; //Устанавливаем флаг корректности в полученное выше значение
                    checkWork.notify_all();
                }
            }
        }
        countOfProgramms++; //Увеличиваем количество написанных программ на 1
        std::printf("Prog %d: \t%d programs written\n", index, countOfProgramms);
    }
}

/**
 * Считывает число
 * @param minValue минимальное значение
 * @param maxValue максимальное значение
 * @return считанное число
 */
int readNumber(int minValue, int maxValue) {
    std::printf("Write an amount of needed correct programs from 1 to 100\n");
    int number;
    std::cin >> number;
    if (number < minValue || number > maxValue) {
        std::cout << "Incorrect input";
        return -1;
    }
    return number;
}

int main() {
    int temp = readNumber(1, 100); //Считываем количество программ, которое нужно написать
    if (temp == -1) {
        std::printf("Incorrect input");
        return 0;
    }
    int countProgs = temp;
    std::thread* threads = new std::thread[3]; //Создаем потоки программистов
    for (int i = 0; i < 3; ++i) {
        isReady.push_back(false);
        isCorrect.push_back(false);
        threads[i] = std::thread(programmer, i);
    }

    while (countOfProgramms < countProgs) //Ждем пока программисты напишут нужное количество программ
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    for (int i = 0; i < 3; ++i) { // Выводим потоки из бесконечного и удаляем их
        threads[i].detach();
    }

    delete[] threads;
    return 0;
}
