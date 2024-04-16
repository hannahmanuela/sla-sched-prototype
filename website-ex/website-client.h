#include <vector>
#include <string>

#ifndef WEBSITE_H
#define WEBSITE_H

class Website {

    public:
        Website() {}

        void run();


    private:
        void gen_load(int lb_conn_fd);

        int connect_to_lb_();

};

#endif  // WEBSITE_H