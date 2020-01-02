[![EOSIO](https://img.shields.io/badge/eosio.cdt-1.6.2-brightgreen)]() [![](https://img.shields.io/badge/EOSIO-Jungle-blue)](https://monitor.jungletestnet.io)

# betgame
This project is a simple smart contract of EOSIO. 

**First use editeam action add team**
```
cleos push action [deployaccount] editeam '["create", "superman", "team description"]' -p [deployaccount]
cleos push action [deployaccount] editeam '["create", "batman", "team description"]' -p [deployaccount]
```

**And use deploy account start new game**
```
cleos push action [deployaccount] start '["superman", "batman", "betgameadmin"]' -p [deployaccount]
```

**When game is started, now everyone can bet by transfer action**
```
cleos transfer eosioaccount [deployaccount] '1 EOS' 'superman'
```

**Now admin must be set game status playing, and everyone can't bet**
```
cleos push action [deployaccount] play '[GameId]' -p [betgameadmin]
```

**When game result come out, Just lottery with game id**
```
cleos push action [deployaccount] lottery '[1, "superman"]' -p [betgameadmin]
```

**And then winner could be claim reward from this game**
```
cleos push action [deployaccount] claim '[1, "eosioaccount"]' -p eosioaccount
```