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
        check(game != games.end(), "game does't exists");
        check(game->status == 1, "game playing or game over");
        check((game->redteam == winner || game->blueteam == winner), "team is not playing");

        record_index records(get_self(), game->id);
        auto rowid = records.available_primary_key();
        rowid = rowid > 0 ? rowid : 1;

        auto timestamp = now();
        auto temp = zero();
        records.emplace(get_self(), [&](auto& row){

            row.id = rowid;
            row.user = from;
            row.winner = winner;
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

    // start a game （only
    void betgame::start(name red, name blue, name admin){
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
            row.admin = admin;
            row.timestamp = timestamp;
            row.ramount = temp;
            row.bamount = temp;
            row.status = 1;
        });

        teams.modify(ft, get_self(), [&](auto& row){

            row.gameid = rowid;
            row.playing = true;
        });

        teams.modify(st, get_self(), [&](auto& row){

            row.gameid = rowid;
            row.playing = true;
        });
    }

    void betgame::play(uint64_t gameid){

        auto game = games.find(gameid);
        check(game != games.end(), "Game don't exists");
        check(game->status == 1, "game over");

        require_auth(game->admin);

        auto timestamp = now();
        games.modify(game, get_self(), [&](auto& row){

            row.status = 2;
            row.timestamp = timestamp;
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

    // 发起结果录入，只有指定的管理员能进行操作
    void betgame::lottery(uint64_t gameid, name winner){

        auto team = teams.find(winner.value);
        check(team != teams.end(), "Team don't exists");

        auto game = games.find(gameid);
        check(game != games.end(), "Game don't exists");
        check(game->status == 2, "game over or game not start");
        check((game->redteam == winner || game->blueteam == winner), "winner don't playing");

        require_auth(game->admin);

        auto timestamp = now();
        games.modify(game, get_self(), [&](auto& row){

            row.status = 3;
            row.winner = winner;
            row.timestamp = timestamp;
        });

        auto redteam = teams.find(game->redteam.value);
        if (redteam != teams.end()){

            teams.modify(redteam, get_self(), [&](auto& row){
                row.gameid = 0;
                row.playing = false;
            });
        }

        auto blueteam = teams.find(game->blueteam.value);
        if (blueteam != teams.end()){
            teams.modify(blueteam, get_self(), [&](auto& row){

               row.gameid = 0;
               row.playing = false;
            });
        }

    }

    void betgame::claim(uint64_t gameid, name account){

        require_auth(account);

        // 查询游戏信息
        auto game = games.find(gameid);
        check(game != games.end(), "Game don't exists");
        check(game->status == 3, "Game does'n end");

        // 查询用户信息
        auto user = users.find(account.value);
        check(user != users.end(), "user don't exists");

        // 找出投注记录
        record_index records(get_self(), gameid);
        auto secondary = records.get_index<name("user")>();
        auto bet_records = secondary.find(account.value);

        auto total = game->ramount + game->bamount;
        auto pool = game->ramount;
        if (game->blueteam == game->winner){

            pool = game->bamount;
        }

        // 判断胜利的记录
        while (bet_records != secondary.end() && bet_records->user == account){

            print("ratio:", bet_records->quantity.amount * 1.0 / pool.amount, "\n");
            if (game->winner == bet_records->winner && bet_records->claimed == false){

                auto reward_amount = total.amount * (bet_records->quantity.amount * 1.0 / pool.amount);
                auto reward = asset(reward_amount, EOS_SYMBOL);

                users.modify(user, get_self(), [&](auto& row){

                    row.reward += reward;
                    row.totalreward += reward;
                });

               auto record = records.find(bet_records->id);
               records.modify(record, get_self(), [&](auto& row){

                    row.claimed = true;
               });
            }

            bet_records++;
        }

        user = users.find(account.value);
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

        record_index records(get_self(), gameid);
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
