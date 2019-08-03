#include "betgame.hpp"

namespace gameio {

    void betgame::bet(name from, name to, asset quantity, string memo){

        if (from == get_self() || to != get_self()){

            return;
        }
        name winner = name(memo);
        auto team = teams.find(winner.value);
        check(team != teams.end(), "team dosn't exists");

        auto game = games.find(team->gameid);
        check(game != games.end(), "");
        check(game->status == 1, "game over");
        check((game->redteam == winner || game->blueteam == winner), "team is not playing");

        record_index records(get_self(), winner.value);
        auto rowid = records.available_primary_key();
        rowid = rowid > 0 ? rowid : 1;

        auto timestamp = now();
        auto temp = zero();
        records.emplace(get_self(), [&](auto& row){

            row.id = rowid;
            row.user = from;
            row.quantity = quantity;
            row.timestamp = timestamp;
        });

        // update game info
        if (game->redteam == winner){

            games.modify(game, get_self(), [&](auto& row){

                row.ramount += quantity;
                row.rusercount += 1;
            });
        }else{

            games.modify(game, get_self(), [&](auto& row){

                row.bamount += quantity;
                row.busercount += 1;
            });
        }

        // update user info
        auto user = users.find(from.value);
        if (user == users.end()){

            users.emplace(get_self(), [&](auto& row){

                row.name = from;
                row.count = 1;
                row.amount = quantity;
                row.reward = temp;
                row.totalreward = temp;
                row.reg_time = timestamp;
            });
        }else{

            users.modify(user, get_self(), [&](auto& row){

                row.count += 1;
                row.amount += quantity;
            });
        }
    }

    void betgame::start(name red, name blue){
        auto ft = teams.find(red.value);
        auto st = teams.find(blue.value);

        check((ft != teams.end() && st != teams.end()), "team don't exists");
        check((ft->playing == false && st->playing == false), "team is playing can't start new");

        auto rowid = games.available_primary_key();
        rowid = rowid > 0 ? rowid : 1;

        auto timestamp = now();
        auto temp = zero();
        games.emplace(get_self(), [&](auto& row){

            row.id = rowid;
            row.redteam = red;
            row.blueteam = blue;
            row.timestamp = timestamp;
            row.ramount = temp;
            row.bamount = temp;
            row.status = 1;
        });

        teams.modify(ft, get_self(), [&](auto& row){

            row.gameid = rowid;
        });

        teams.modify(st, get_self(), [&](auto& row){

            row.gameid = rowid;
        });
    }

    void betgame::editeam(string action, name title, string description){

        require_auth(get_self());

        auto team = teams.find(title.value);
        if (action == "delete"){

            check(team != teams.end(), "Team don't exists");
            teams.erase(team);
            print("Delete succeed");

       }else if(action == "create"){

            check(team == teams.end(), "Team already created");

            teams.emplace(get_self(), [&](auto& row){

                row.title = title;
                row.description = description;
                row.playing = false;
            });
            print("Create complete: ", title);

        }else{

            print("Unsupported action");
        }
    }

    void betgame::lottery(uint64_t gameid, name winner){
        require_auth(get_self());

        auto team = teams.find(winner.value);
        check(team != teams.end(), "Team don't exists");

        auto game = games.find(gameid);
        check(game != games.end(), "Game don't exists");
        check(game->status == 1, "Game over");
        check((game->redteam == winner || game->blueteam == winner), "winner don't playing");

        calculate(gameid, winner);

        auto timestamp = now();
        games.modify(game, get_self(), [&](auto& row){

            row.status = 2;
            row.winner = winner;
            row.timestamp = timestamp;
        });

        // delete all record
        clear(game->redteam, game->id);
        clear(game->blueteam, game->id);
    }

    void betgame::calculate(uint64_t gameid, name winner){

        auto game = games.find(gameid);
        record_index records(get_self(), winner.value);

        for (auto &record : records){

            auto total = game->ramount + game->bamount;
            auto user = users.find(record.user.value);
            auto pool = game->ramount;

            if (game->blueteam == winner){

                pool = game->bamount;
            }

            auto reward = total / pool * record.quantity;
            users.modify(user, get_self(), [&](auto& row){

                row.reward += reward;
                row.totalreward += reward;
            });
        }
    }

    void betgame::claim(name account){

        require_auth(account);

        auto user = users.find(user.value);
        check(user != users.end(), "user don't exists");
        check(user->reward.amount > 0, "no reward for claim");

        auto temp = zero();
        auto timestamp = now();

        // send EOS
        send(EOS_TOKEN_CONTRACT, account, user->reward, "claim");

        users.modify(user, get_self(), [&](auto& row){

            row.reward = temp;
            row.claim_time = timestamp;
        });
    }

    void betgame::send(name contract, name user, asset quantity, string memo){

        auto data = std::make_tuple(get_self(), user, quantity, memo);

        action(permission_level{get_self(), name("active")}, contract, name("transfer"), data).send();
    }

    uint64_t betgame::now(){

        return current_time_point().sec_since_epoch();
    }

    asset betgame::zero(){

        return asset(0, EOS_SYMBOL);
    }

    void betgame::clear(name team, uint64_t gameid){

        record_index records(get_self(), team.value);
        std::vector <uint64_t> keyForRecords;
        for (auto &record : records) {

            keyForRecords.push_back(record.id);
        }

        for (uint64_t id : keyForRecords) {

            auto record = records.find(id);
            if (record != records.end()) {

                records.erase(record);
            }
        }
    }
}
