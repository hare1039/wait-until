#include <iostream>
#include <string>
#include <algorithm>
#include <vector>
#include <iomanip>
#include <cstdlib>
#include <csignal>

#include <boost/asio.hpp>
#include <boost/scope_exit.hpp>
#include <boost/filesystem.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/process.hpp>
#include <boost/program_options.hpp>
#include <boost/regex.hpp>

class process_monitor
{
    boost::asio::io_context& io_;
    boost::process::async_pipe run_pipe_;
    boost::asio::streambuf run_stdout_;
    boost::process::child run_child_;

    std::string exec_process_;
    std::vector<std::string> exec_argv_;

    boost::regex key_regex_;

public:
    process_monitor(std::string const &run_argv,
                    std::string const &regex_str,
                    std::vector<std::string> const& exec_argv,
                    boost::asio::io_context& io)
        :io_{io},
         run_pipe_{io_},
         run_child_(run_argv,
                    boost::process::std_out > run_pipe_,
                    boost::process::std_err > stderr,
                    io_),
         key_regex_{regex_str}
    {
        exec_process_ = exec_argv.front();
        std::copy(std::next(exec_argv.begin()), exec_argv.end(), std::back_inserter(exec_argv_));
    }

    void start_read()
    {
        namespace bp = boost::process;
        boost::asio::async_read_until(
            run_pipe_,
            run_stdout_,
            key_regex_,
            [this](boost::system::error_code const &error, std::size_t) {
                if (not error || error == boost::asio::error::eof)
                    start_exec();
                else
                    std::cerr << error.message() << "\n";
            });
    }

    void start_exec()
    {
        namespace bp = boost::process;
        bp::child c(bp::search_path(exec_process_), bp::args(exec_argv_),
                    bp::std_in  < stdin,
                    bp::std_out > stdout,
                    bp::std_err > stderr);
        c.wait();
    }

private:

};

int main(int argc, char* argv[])
{
    namespace po = boost::program_options;
    namespace fs = boost::filesystem;
    try
    {
        std::string regex, run_argv;
        std::vector<std::string> exec_args;
        po::options_description desc("Allowed options");
        desc.add_options()
            ("help", "produce help message")
            ("run", po::value<std::string>(&run_argv)->required(), "Monitor this command")
            ("regex", po::value<std::string>(&regex)->required(), "Wait until this regex shows up")
            ("command", po::value<std::vector<std::string>>(&exec_args)->required(), "Run this command when keyword shows up")
            ;

        po::positional_options_description p;
        p.add("command", -1);
        po::variables_map vm;
        po::parsed_options parsed = po::command_line_parser(argc, argv).options(desc).positional(p).run();
        po::store(parsed, vm);

        if (vm.count("help"))
        {
            std::cout << desc << '\n';
            std::exit(EXIT_SUCCESS);
        }

        po::notify(vm);
        boost::asio::io_context io;

        process_monitor pm {run_argv, regex, exec_args, io};
        pm.start_read();
        io.run();
    }
    catch(std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
