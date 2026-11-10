#include <cstdio>
#include <cstring>
#include <pthread.h>
#include <unistd.h>

struct Payload {
    int value;
};

struct Event {
    int number;
    Payload* data;
};

class Monitor {
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t condition = PTHREAD_COND_INITIALIZER;
    Event* event = nullptr;
    bool ready = false;
    bool stopped = false;

public:
    ~Monitor() {
        pthread_cond_destroy(&condition);
        pthread_mutex_destroy(&mutex);
    }

    void provide(Event* new_event) {
        pthread_mutex_lock(&mutex);
        while (ready) pthread_cond_wait(&condition, &mutex);

        event = new_event;
        ready = true;
        std::printf("Поставщик: отправил событие %d\n", event->number);
        pthread_cond_signal(&condition);
        pthread_mutex_unlock(&mutex);
    }

    bool consume() {
        pthread_mutex_lock(&mutex);
        while (!ready && !stopped) pthread_cond_wait(&condition, &mutex);

        if (!ready) {
            pthread_mutex_unlock(&mutex);
            return false;
        }

        std::printf("Потребитель: обработал событие %d, данные %d\n",
                    event->number, event->data->value);
        delete event->data;
        delete event;
        event = nullptr;
        ready = false;
        pthread_cond_signal(&condition);
        pthread_mutex_unlock(&mutex);
        return true;
    }

    void stop() {
        pthread_mutex_lock(&mutex);
        stopped = true;
        pthread_cond_broadcast(&condition);
        pthread_mutex_unlock(&mutex);
    }
};

void* producer(void* argument) {
    auto* monitor = static_cast<Monitor*>(argument);
    for (int i = 1; i <= 5; ++i) {
        sleep(1);
        monitor->provide(new Event{i, new Payload{i * 10}});
    }
    return nullptr;
}

void* consumer(void* argument) {
    auto* monitor = static_cast<Monitor*>(argument);
    for (int i = 0; i < 5; ++i) {
        if (!monitor->consume()) break;
    }
    return nullptr;
}

int main() {
    Monitor monitor;
    pthread_t consumer_thread;
    pthread_t producer_thread;

    int error = pthread_create(&consumer_thread, nullptr, consumer, &monitor);
    if (error != 0) {
        std::fprintf(stderr, "pthread_create (consumer): %s\n", std::strerror(error));
        return 1;
    }

    error = pthread_create(&producer_thread, nullptr, producer, &monitor);
    if (error != 0) {
        std::fprintf(stderr, "pthread_create (producer): %s\n", std::strerror(error));
        monitor.stop();
        pthread_join(consumer_thread, nullptr);
        return 1;
    }

    pthread_join(producer_thread, nullptr);
    pthread_join(consumer_thread, nullptr);
    return 0;
}
