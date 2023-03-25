#include <Server/Server.hh>

#include <thread>

#include <Server/Simulation.hh>

namespace app
{
    Server::Server()
        : m_Simulation(*this)
    {
        // websocket++ spams stdout by default (goofy)
        m_Server.set_access_channels(websocketpp::log::alevel::none);
        m_Server.init_asio();
        m_Server.listen(8000);
        m_Server.start_accept();
        m_Server.set_open_handler(bind(&Server::OnClientConnect, this, websocketpp::lib::placeholders::_1));
        m_Server.set_close_handler(bind(&Server::OnClientDisconnect, this, websocketpp::lib::placeholders::_1));
        m_Server.set_message_handler(bind(&Server::OnMessage, this, websocketpp::lib::placeholders::_1, websocketpp::lib::placeholders::_2));

        std::thread([&]()
                    {
            using namespace std::chrono_literals;
            while (true)
            {
                Tick();
                std::this_thread::sleep_for(40ms);
            } })
            .detach();
    }

    void Server::Tick()
    {
        m_Simulation.Tick();

        for (Client *client : m_Clients)
            client->Tick();
    }

    void Server::Run()
    {
        m_Server.run();
    }

    void Server::OnClientConnect(websocketpp::connection_hdl hdl)
    {
        m_Clients.push_back(new Client(hdl, m_Simulation));
    }

    void Server::OnClientDisconnect(websocketpp::connection_hdl hdl)
    {
        for (decltype(m_Clients)::iterator i = m_Clients.begin(); i != m_Clients.end(); i++)
            if ((*i)->GetHdl().lock() == hdl.lock())
            {
                delete *i;
                m_Clients.erase(i);
                break;
            }
    }

    void Server::OnMessage(websocketpp::connection_hdl hdl, WebSocketServer::message_ptr message)
    {
        for (decltype(m_Clients)::iterator i = m_Clients.begin(); i != m_Clients.end(); i++)
            if ((*i)->GetHdl().lock() == hdl.lock())
            {
                (*i)->ReadPacket((uint8_t *)message->get_raw_payload().c_str(), message->get_raw_payload().size());
            }
    }
}