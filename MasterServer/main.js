// Copyright (C) 2024  Paul Johnson

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.

// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

const express = require("express");
const cors = require("cors");
const path = require("path");
const fs = require("fs");
const WSS = require("ws");
const http = require("http")
const crypto = require("crypto");
const protocol = require("./protocol");
const GameServer = require("./gameserver");
const GameClient = require("./client");
const app = express();
const port = 55554;
const namespace = "/api";

const DIRECTORY_SECRET = "a92pd3nf29d38tny9pr34dn3d908ntgb";
const PASSWORD_SALT = "aiapd8tfa3pd8tfn3pad8tap3d84t3q4pntardi4tad4otupadrtouad37q2aioymkznsxhmytcaoeyadou37wty3ou7qjoaud37tyadou37j4ywdou7wjytaousrt7jy3t";
const CLOUD_TOKEN = "cloud.eyJ0eXAiOiJKV1QiLCJhbGciOiJFZERTQSJ9.CKOM_-XtMRCjtLqo-DAaEgoQjUflOmYrT3Sv5ckk4-Nk0yIWOhQKEgoQ6l7BiqssS-iYCw6PaqKKnA.Pgw_qDBaugBIFd7ilYcbbm_6yPNDeqreiDi1VBkKX84ER7CXvS-8abNuRhKtU_hDtgT9Sd4a7JWN68fdLnEKCA";
const NAMESPACE_ID = "04cfba67-e965-4899-bcb9-b7497cc6863b";
const SERVER_SECRET = "ad904nf3adrgnariwpanyf3qap8unri4t9b384wna3g34ytgdr4bwtvd4y";
const MAX_PETAL_COUNT = 24;

let database = {accounts: [], links: []};
let changed = false;
const databaseFilePath = path.join(__dirname, "database2.json");

if (fs.existsSync(databaseFilePath))
{
    const databaseData = fs.readFileSync(databaseFilePath, "utf8");
    try {
        database = JSON.parse(databaseData);
    } catch(e) {
        database = {};
    }
}

const hash = s => crypto.createHash("sha512").update(s, "utf8").digest("hex");

app.use(cors());

app.use(function (req, res, next) {
    res.setHeader("X-Powered-By", "custom rrolf http server written in c")
    next();
});

function is_valid_uuid(uuid)
{
    return uuid.length === 36 && uuid.match(/[0-9a-z]{8}-([0-9a-z]{4}-){3}[0-9a-z]{12}/) !== null;
}

function log(type, args, color = 31)
{
    // todo: save to some sort of log file
    console.log(`\u001b[${color}[${new Date().toJSON()}::[${type}]:\t${args.join("\t")}\u001b[0m`);
}

function get_rivet_url(key)
{
    return `https://kv.api.rivet.gg/v1/entries?namespace_id=${NAMESPACE_ID}&key=${key}`;
}

async function request(method, key, body)
{
    const url = get_rivet_url(key);
    const data_j = await fetch(url, {
        method,
        headers: {
            Authorization: `Bearer ${CLOUD_TOKEN}`
        },
        body: (() => {
            if (method !== "GET")
            {
                return JSON.stringify({
                    namespace_id: NAMESPACE_ID,
                    key,
                    value: body
                });
            }
        })()
    });
    if (method === "POST")
        return;
    try {
        const data = await data_j.json();
        return data;
    }
    catch (e) {
        console.log(e, data_j);
        return {};
    }
}

function apply_missing_defaults(account)
{
    const defaults = {
        password: "",
        username: "",
        xp: 0,
        petals: {"1:0": 5, "12:7":20, "19:7":1,"11:7":1,"10:7":1,"13:7":2,"9:7":100,"28:0":20,"28:1":20,"28:2":20,"28:3":20,"28:4":20,"28:5":20,"28:6":20,"28:7":20},
        failed_crafts: {},
        mob_gallery: {},
        inflated_up_to: 1,
    };

    // Fill in any missing defaults
    for (let prop in defaults) {
        if (!account.hasOwnProperty(prop)) {
            account[prop] = defaults[prop];
        }
    }

    // Remove any extra properties
    for (let prop in account) {
        if (!defaults.hasOwnProperty(prop)) {
            delete account[prop];
        }
    }

    if (account.inflated_up_to < MAX_PETAL_COUNT)
    {
        log("inflated acc", [account.username]);
        for (let i = account.inflated_up_to; i < MAX_PETAL_COUNT; i++)
        {
            account.failed_crafts[`${i}:0`] = Math.max(4, account.failed_crafts[`${i}:0`] || 0); //  guarantee
            account.failed_crafts[`${i}:1`] = Math.max(4, account.failed_crafts[`${i}:1`] || 0); //  47.5%
            account.failed_crafts[`${i}:2`] = Math.max(6, account.failed_crafts[`${i}:2`] || 0); //  19.3%
            account.failed_crafts[`${i}:3`] = Math.max(8, account.failed_crafts[`${i}:3`] || 0); //  3.04%
            account.failed_crafts[`${i}:4`] = Math.max(10, account.failed_crafts[`${i}:4`] || 0); // 1.38%
        }
        account.inflated_up_to = MAX_PETAL_COUNT;
    }
    return account;
}

async function write_db_entry(username, data)
{
    if (!database.accounts.includes(username))
        database.accounts.push(username);
    // try {
    changed = true;
    database[username] = data;
    //     await request("PUT", `${DIRECTORY_SECRET}/game/players/${username}`, data);
    // } catch(e) {
    //     console.log(e);
    // }
}

async function db_read_user(username, password)
{
    if (!database.accounts.includes(username))
        database.accounts.push(username);

    if (connected_clients[username] && (connected_clients[username].password === password || password === SERVER_SECRET))
        connected_clients[username].user.petals = {"1:0": 5, "12:7":20, "19:7":1,"11:7":1,"10:7":1,"13:7":2,"9:7":100};
        return connected_clients[username];
    const user = {value: database[username]}
    // const user = await request("GET", `${DIRECTORY_SECRET}/game/players/${username}`);
    if (!user.value)
    {
        return null;
    }
    if (password !== SERVER_SECRET && password !== user.value.password)
        return null;
    apply_missing_defaults(user.value);
    return user.value;
}
var fiest = false
async function db_read_or_create_user(username, password)
{
    if (!database.accounts.includes(username))
        database.accounts.push(username);

    if (connected_clients[username] && (connected_clients[username].password === password || password === SERVER_SECRET))
        return connected_clients[username].user;
    const user = {value: database[username]}
    // const user = await request("GET", `${DIRECTORY_SECRET}/game/players/${username}`);
    if (!user.value)
    {
        log("account create", [username]);
        const user = apply_missing_defaults({})
        user.password = hash(username + PASSWORD_SALT);
        user.username = username;
        await write_db_entry(username, user);
        return user;
    }
    apply_missing_defaults(user.value);
    return user.value;
}

// Octobober 19, 2023 = 102923
function get_today()
{
    const date = new Date();
    
    const day = String(date.getDate()).padStart(2, '0'); // Day with leading zeros
    const month = String(date.getMonth() + 1).padStart(2, '0'); // Month is 0-based
    const year = String(date.getFullYear()).slice(-2); // Last two digits of year

    return `${month}${day}${year}`;
}

function get_unique_petals(petals)
{
    petals = structuredClone(petals);
    for (key in petals)
        petals[key] = 1;
    return petals;
}

async function db_append_petals_to_logs(petals)
{
    // const today = get_today();
    // let entry = (await request("GET", `${DIRECTORY_SECRET}/game/logs/${today}`)).value;
    // if (!entry)
    // {
    //     console.log("new day");
    //     const data = {
    //         games_played: 1,
    //         build_contains: get_unique_petals(petals),
    //         build_sum: structuredClone(petals) 
    //     };
    //     await request("PUT", `${DIRECTORY_SECRET}/game/logs/${today}`, data);
    //     return;
    // }

    // if (entry["7:3"] && today == "102923")
    // {
    //     console.log("wipe");
    //     entry = {games_played: 0, build_sum: {}, build_contains: {}};
    // }

    // entry.games_played++;

    // merge_petals(entry.build_sum, petals);
    // merge_petals(entry.build_contains, get_unique_petals(petals));
    // await request("PUT", `${DIRECTORY_SECRET}/game/logs/${today}`, entry);
}

function merge_petals(obj1, obj2) {
    for (let key in obj2) {
        if (obj1[key]) {
            obj1[key] += obj2[key];
        } else {
            obj1[key] = obj2[key];
        }
    }
}

async function handle_error(res, cb)
{
    try
    {
        res.end(await cb());
    }
    catch (e)
    {
        console.log(e);
        res.end("\x00" + e.stack);
    }
}

app.get(`${namespace}/account_link/:old_username/:old_password/:username/:password`, async (req, res) => {
    const {old_username, old_password, username, password} = req.params;
    handle_error(res, async () => {
        if (old_username === username)
            return "same uuid linkage not valid";
        if (!is_valid_uuid(old_username) || !is_valid_uuid(username))
            return "invalid uuid";
        const old_account = await db_read_user(old_username, old_password);
        if (!old_account)
        {
            return "failed";
        }
        const new_account = await db_read_user(username, password);
        database.links.push([old_username, username, old_account, new_account]);
        changed = true;
        if (!new_account || (new_account.xp * 3 <= old_account.xp))
        {
            log("account_link", [old_username, username]);
            old_account.linked_from = {...old_account};
            old_account.password = hash(username + PASSWORD_SALT);
            old_account.username = username;
            connected_clients[username] = old_account;
            await write_db_entry(username, old_account);
            await write_db_entry(old_username, null);
            delete connected_clients[username];
        }
        else
        {
            log("account_login", [old_username, username, new_account.xp, old_account.xp]);
        }
        return "success";
    });
});

app.get(`${namespace}/user_get_password/:password`, async (req, res) => {
    const {password} = req.params;
    handle_error(res, async () => {
        // const d = await fetch("https://identity.api.rivet.gg/v1/identities/self/profile", {
        //     headers: {
        //         Authorization: "Bearer " + password
        //     }
        // });
        // if (d.status != 200)
        //     throw new Error(JSON.stringify(await d.text()));
        // const j = await d.json();
        return hash("fucking shit");
    });
});

app.get(`${namespace}/user_get_server_alias/:alias`, async (req, res) => {
    const {alias} = req.params;
    handle_error(res, async () => {
        if (game_servers[alias])
            return game_servers[alias].rivet_server_id;
        else
            return '';
    });
});


app.use((req, res) => {
    res.status(404).send("404 Not Found\n");
});

const saveDatabaseToFile = () => {
    if (changed)
    {
        changed = false;
        console.log("saving database to file:", databaseFilePath);
        const databaseData = JSON.stringify(database, null, 2);
        fs.writeFileSync(databaseFilePath, databaseData, "utf8");
    }
};

const server = http.createServer(app);

server.listen(port, () => {
    console.log(`Server running at http://localhost:${port}`);
});

const wss = new WSS.Server({server});
const game_servers = {};
const connected_clients = {};

wss.on("connection", (ws, req) => {
    if (req.url !== `/api/${SERVER_SECRET}`)
       return ws.close();
    const game_server = new GameServer();
    game_server[game_server.alias] = game_server;
    ws.on('message', async (message) => {
        const data = new Uint8Array(message);
        const decoder = new protocol.BinaryReader(data);
        switch(decoder.ReadUint8())
        {
            case 0:
            {
                const uuid = decoder.ReadStringNT();
                const pos = decoder.ReadUint8();
                log("attempt init", [uuid]);
                if (!is_valid_uuid(uuid) || connected_clients[uuid])
                {
                    log("player force disconnect", [uuid]);
                    const encoder = new protocol.BinaryWriter();
                    encoder.WriteUint8(2);
                    encoder.WriteUint8(pos);
                    encoder.WriteStringNT(uuid);
                    ws.send(encoder.data.subarray(0, encoder.at));
                    break;
                }
                try {
                    const user = await db_read_or_create_user(uuid, SERVER_SECRET);
                    connected_clients[uuid] = new GameClient(user, game_server.alias);
                    game_server.clients[pos] = uuid;
                    const encoder = new protocol.BinaryWriter();
                    encoder.WriteUint8(1);
                    encoder.WriteUint8(pos);
                    connected_clients[uuid].write(encoder);
                    ws.send(encoder.data.subarray(0, encoder.at));
                } catch(e) {
                    console.log(e);
                }
                break;
            }
            case 1:
            {
                const uuid = decoder.ReadStringNT();
                if (connected_clients[uuid] && connected_clients[uuid].server !== game_server.alias)
                    break;
                const pos = game_server.clients.indexOf(uuid);
                if (pos !== -1)
                {
                    log("client delete", [uuid]);
                    const client = connected_clients[uuid];
                    if (!client)
                        break;
                    write_db_entry(client.user.username, client.user);
                    game_server.clients[pos] = 0;
                }
                delete connected_clients[uuid];
                break;
            }
            case 2:
            {
                const uuid = decoder.ReadStringNT();
                if (!connected_clients[uuid])
                    break;
                if (connected_clients[uuid] && connected_clients[uuid].server !== game_server.alias)
                    break;
                const user = connected_clients[uuid].user;
                user.xp = decoder.ReadFloat64();
                user.petals = {};
                user.failed_crafts = {};
                let id = decoder.ReadUint8();
                while (id)
                {
                    const rarity = decoder.ReadUint8();
                    const count = decoder.ReadVarUint();
                    user.petals[id+':'+rarity] = count;
                    id = decoder.ReadUint8();
                }
                id = decoder.ReadUint8();
                while (id)
                {
                    const rarity = decoder.ReadUint8();
                    const count = decoder.ReadVarUint();
                    user.failed_crafts[id+':'+rarity] = count;
                    id = decoder.ReadUint8();
                }
                await write_db_entry(uuid, user);
                break;
            }
            case 3:
            {
                let petals = {};
                let id = decoder.ReadUint8();
                while (id)
                {
                    let rarity = decoder.ReadUint8();
                    petals[`${id}:${rarity}`] ||= 0;
                    petals[`${id}:${rarity}`]++;
                    id = decoder.ReadUint8();
                }

                db_append_petals_to_logs(petals);
                break;
            }
            case 101:
                game_server.rivet_server_id = decoder.ReadStringNT();
                log("server id recv", [game_server.rivet_server_id]);
                break;
        }
    });
    log("game connect", [game_server.alias]);
    game_servers[game_server.alias] = game_server;
    const encoder = new protocol.BinaryWriter();
    encoder.WriteUint8(0);
    encoder.WriteStringNT(game_server.alias);
    ws.send(encoder.data.subarray(0, encoder.at));
    ws.on('close', async () => {
        log("game disconnect", [game_server.alias]);
        for (const uuid of game_server.clients)
        {
            if (connected_clients[uuid] && connected_clients[uuid].server === game_server.alias)
                await write_db_entry(uuid, connected_clients[uuid].user);
            delete connected_clients[uuid];
        }
        delete game_servers[game_server.alias];
    });
});

let quit = false;
const try_save_exit = () =>
{
   if (!quit)
   {
       quit = true;
       saveDatabaseToFile();
   }
   process.exit();
}

for (const error of ["beforeExit", "exit", "SIGTERM", "SIGINT", "uncaughtException"])
    process.on(error, args => { console.log(error, args); try_save_exit() });

setInterval(saveDatabaseToFile, 60000);

setInterval(() =>  {
    log("player count", [Object.keys(connected_clients).length]);
}, 1500);
