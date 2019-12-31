#include <eosio/transaction.hpp>
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/print.hpp>
#include <string>
#include <vector>

using namespace eosio;

#define EOS_TOKEN_CONTRACT name("eosio.token")
#define EOS_SYMBOL symbol("EOS", 4)

namespace gameio{

    using std::string;
    using eosio::current_time_point;

    CONTRACT betgame: public eosio::contract{

    public:
        using contract::contract;
        betgame(name receiver, name code, datastream<const char*> ds):
        contract(receiver, code, ds),
        teams(receiver, get_self().value),
        users(receiver, get_self().value),
        games(receiver, get_self().value){}

        [[eosio::on_notify("eosio.token::transfer")]]
        void bet(name from, name to, asset quantity, string memo);

        ACTION editeam(string action, name title, string description);
        ACTION start(name red, name blue, name admin);
        ACTION lottery(uint64_t gameid, name winner);
        ACTION claim(uint64_t gameid, name account);

        void send(name contract, name user, asset quantity, string memo);
        void clear(name team,uint64_t gameid);
        asset zero();
        uint64_t now();

    private:
        TABLE team {

            name title;
            string description;
            uint64_t gameid;

            bool playing;

            uint128_t primary_key() const {

                return title.value;
            }
        };

        typedef eosio::multi_index<name("team"), team> team_index;
        team_index teams;

        TABLE user {

            name name;
            uint32_t count;
            asset amount;
            asset reward;
            asset totalreward;
            uint64_t claim_time;
            uint64_t reg_time;

            uint128_t primary_key() const{

                return name.value;
            }
        };

        typedef eosio::multi_index<name("user"), user> user_index;
        user_index users;

        TABLE game {

            uint64_t id;
            name redteam;
            name blueteam;
            name winner;
            name admin;
            asset ramount;
            asset bamount;
            uint64_t rusercount;
            uint64_t busercount;
            uint64_t timestamp;
            uint8_t status;

            uint64_t primary_key() const {

                return id;
            }
        };
        typedef eosio::multi_index<name("game"), game> game_index;
        game_index games;

        TABLE record {

            uint64_t id;
            name user;
            name winner;
            asset quantity;
            uint64_t timestamp;
            asset reward;
            bool claimed;

            uint64_t primary_key() const {

                return id;
            }

            uint64_t user_key() const {

                return user.value;
            }
        };
        typedef eosio::multi_index<name("record"), record, indexed_by<name("user"), const_mem_fun<record, uint64_t, &record::user_key>>> record_index;
    };

}
