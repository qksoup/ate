#include <ate/ate.hpp>

#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

using std::cout;
using std::endl;

namespace {
    class SigwaitThread;
    typedef void (*SignalHandler)(int);
    
    bool inited = false;
    SigwaitThread* sigwaitThread;
    ate::SpinLock lock;
    sigset_t sgset;
    enum {
        SIGNAL_NUMBER_MAX = 128
    };
    SignalHandler signalHandlers[SIGNAL_NUMBER_MAX];
    
    class SigwaitThread : public ate::Thread {
    public:        
        virtual void run();        
    };
    
    // sigwait() and process signals
    void SigwaitThread::run()
    {
        int signo;        
        while (true) {
            sigwait(&sgset, &signo);    
            cout << "received signal " << signo << ": " << strsignal(signo) 
                << endl; //todo: log
            if (signalHandlers[signo] != 0) {
                signalHandlers[signo](signo);
            }
        }
    }
    
    void handleSigChld(int)
    {
        int status;
        pid_t pid;
        while ((pid = waitpid(-1, &status, WNOHANG|WUNTRACED)) > 0) {
            cout << "waitpid() returned for child pid " << pid << endl; //todo:log
        }
    }    
    
    void handleExit(int)
    {
        cout << "signal to exit..." << endl; //todo: log
        exit(0);
    }
    
    void ignoreSignal(int signo)
    {
        cout << "ignore signal " << signo << ": " << strsignal(signo) 
                << endl; //todo: log        
    }
    
    void setSignalHandler(int signo, SignalHandler signalHandler)
    {
        ATE_ASSERT(signo >= 0 && signo < SIGNAL_NUMBER_MAX);
        signalHandlers[signo] = signalHandler;
    }
}

namespace ate {     
    void SignalManager::init() 
    {
        {
            LockGuard<SpinLock> sl(lock);
            if (inited) 
                return;
            inited = true;
        }
        // block signals first. SIGSTOP, SIGCONT and SIGKILL cannot be blocked
        sigemptyset(&sgset);
        sigaddset(&sgset, SIGINT);
        sigaddset(&sgset, SIGTERM);
        sigaddset(&sgset, SIGQUIT);
        sigaddset(&sgset, SIGCHLD); // to avoid zombie process
        sigaddset(&sgset, SIGPIPE); // socket closed by the other end
        ATE_ASSERT(pthread_sigmask(SIG_BLOCK, &sgset, NULL) == 0);   
        
        setSignalHandler(SIGINT, &handleExit);
        setSignalHandler(SIGTERM, &handleExit);
        setSignalHandler(SIGQUIT, &handleExit);
        setSignalHandler(SIGCHLD, &handleSigChld);
        setSignalHandler(SIGPIPE, &ignoreSignal);
        // starts a new thread to receive signal
        sigwaitThread = new SigwaitThread();
        sigwaitThread->start();        
    }
}
