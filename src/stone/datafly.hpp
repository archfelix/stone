#ifndef STONE_DATAFLY_HPP
#define STONE_DATAFLY_HPP

#include <mutex>
#include <queue>
#include <memory>
#include <unordered_map>
#include <functional>

namespace stone
{
    template <class _T>
    using topic_callback = std::function<void(const std::shared_ptr<_T> &)>;

    template <class _T>
    class subscriber
    {
        friend class DataFlyMaster;

    public:
        subscriber(const std::string &topic_name,
                   const topic_callback<_T> &cb,
                   std::size_t _queue_max_size)
        {
            this->topic_name = topic_name;
            this->queue_max_size = _queue_max_size;
            this->callback = cb;
        }
        ~subscriber() {}

        void spin(bool block = false)
        {
            std::shared_ptr<_T> msg = nullptr;

            mtx_msgs.lock();
            if (!msgs.empty())
            {
                msg = msgs.front();
                msgs.pop();
            }
            mtx_msgs.unlock();
            if (msg != nullptr)
            {
                this->callback(msg);
            }
        }

    private:
        bool push(const std::shared_ptr<_T> &msg)
        {
            std::lock_guard<std::mutex> glock(mtx_msgs);
            if (msgs.size() >= this->queue_max_size)
            {
                return false;
            }
            msgs.push(msg);
            return true;
        }

        std::size_t queue_max_size;
        std::string topic_name;

        std::mutex mtx_msgs;
        std::queue<std::shared_ptr<_T>> msgs;

        topic_callback<_T> callback;
    };

    class DataFlyMaster
    {
    public:
        using generic_subscriber = subscriber<void *>;

    public:
        DataFlyMaster() {}
        ~DataFlyMaster() {}

        template <class _T>
        inline void publish(const std::string &topic_name, const std::shared_ptr<_T> &msg)
        {
            std::lock_guard<std::mutex> glock(mtx_subscribers);
            for (auto &&s : subscribers[topic_name])
            {
                subscriber<_T> *sub = reinterpret_cast<subscriber<_T> *>(s);
                sub->push(msg);
            }
        }

        template <class _T>
        inline subscriber<_T> *subscribe(const std::string &topic_name, topic_callback<_T> cb, std::size_t queue_size = 10)
        {
            std::lock_guard<std::mutex> glock(mtx_subscribers);
            subscriber<_T> *s = new subscriber<_T>(topic_name, cb, queue_size);
            this->subscribers[topic_name].push_back(reinterpret_cast<generic_subscriber *>(s));
            return s;
        }

        template <class _T>
        inline bool unsubscribe(subscriber<_T> *_subscriber)
        {
            // Time Complexity: O(n)
            // This can be optimized by using tree, but this function is seldom called.
            std::lock_guard<std::mutex> glock(mtx_subscribers);
            auto &_subers = subscribers[_subscriber->topic_name];
            decltype(_subers.begin()) to_del = _subers.end();
            for (auto i = _subers.begin(); i != _subers.end(); i++)
            {
                if ((*i) == _subscriber)
                {
                    to_del = i;
                    break;
                }
            }
            if (to_del != _subers.end())
            {
                _subers.erase(to_del);
                return true;
            }
            else
            {
                return false;
            }
        }

    private:
        std::mutex mtx_subscribers;
        std::unordered_map<std::string, std::vector<generic_subscriber *>> subscribers;
    };

    extern DataFlyMaster master;

    template <class _T>
    inline void publish(const std::string &topic_name, const std::shared_ptr<_T> &msg)
    {
        master.publish(topic_name, msg);
    }

    template <class _T>
    inline subscriber<_T> *subscribe(const std::string &topic_name, topic_callback<_T> cb, std::size_t queue_size = 10)
    {
        return master.subscribe(topic_name, cb, queue_size);
    }

    template <class _T>
    inline bool unsubscribe(subscriber<_T> *_subscriber)
    {
        return master.unsubscribe(_subscriber);
    }

} // namespace psl

#endif