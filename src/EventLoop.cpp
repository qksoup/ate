#include <ate/ate.hpp>

namespace {
    bool stopLoop(const ate::Timer& timer)
    {
        using ate::EventLoop;
        using ate::Event;
        EventLoop* evloop = static_cast<EventLoop*>(timer.aux);
        evloop->setRunning(false);
        return true; // done
    }
}

namespace ate {    
    void TimerQueue::doAdd(TimerNode* newNode)
    {
        TimerNode* tn;
        for (tn = m_dummy.prev; tn != &m_dummy; tn = tn->prev) {
            if (newNode->timer.time >= tn->timer.time)
                break;
        }
        newNode->prev = tn;
        newNode->next = tn->next;        
        newNode->prev->next = newNode->next->prev = newNode;
    }
    
    void TimerQueue::add(const Timer& timer)
    {        
        TimerNode* newNode = m_pool.alloc();
        new(&newNode->timer) Timer(timer); // copy by default copy constructor
        doAdd(newNode);
    }
    
    void TimerQueue::dispatch(const MicroTime& now)
    {        
        TimerNode* cbNode;
        while ((cbNode = m_dummy.next) != &m_dummy &&
               cbNode->timer.time <= now) {
            m_dummy.next = cbNode->next;
            m_dummy.next->prev = &m_dummy;
            if (cbNode->timer.tcb(cbNode->timer)) { // done. remove
                m_pool.dealloc(cbNode);
            } else { // recurrent timer, add it back to the list
                cbNode->timer.time.add(cbNode->timer.increment);
                doAdd(cbNode);
            }
        }
    }
    
    EventLoop::EventLoop() : m_running(true), m_evq(m_monitor, "EventQueue")
    {
    }
    
    void EventLoop::push(const Event& event)
    {
        m_evq.push(event);
    }
    
    void EventLoop::addTimer(const Timer& timer)
    {
        m_timers.add(timer);
    }
    
    EventNode* EventLoop::getWork(const MicroTime& now, bool& waited)
    {
        LockGuard<Monitor> guard(m_monitor);
        if (now >= m_timers.minTime()) { 
            waited = false;
            return NULL;
        }
        EventNode* en = (EventNode*)m_evq.getWorkWithLock(m_timers.minTime());
        if (en == NULL) // time out from condition wait
            waited = true;
        return en;
    }
    
    void EventLoop::run(int millis)
    {
        m_running = true;
        bool waited;        
        EventNode* eventList;
        if (millis > 0) {
            Timer timer;
            timer.time = MicroTime::now().add(millis);
            timer.increment = millis;
            timer.tcb = stopLoop;
            timer.aux = this;
            m_timers.add(timer);
        }
        MicroTime now;
        while (m_running) { // dispatch timers and events
            now = MicroTime::now();
            eventList = getWork(now, waited);
            begin();
            if (eventList == NULL) { // some timer expired
                if (waited) 
                    now = MicroTime::now();
                m_timers.dispatch(now);
            } else { // got some work from event queue
                for (EventNode* en = eventList; en != NULL; en = en->next) {
                    en->obj.handler->onEvent(en->obj);
                }
            }
            end();
        }
    }
}
