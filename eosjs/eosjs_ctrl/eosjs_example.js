'use strict';

// https://github.com/EOSIO/eosjs
// https://developers.eos.io/
// https://www.eosdocs.io/resources/apiendpoints/
// https://medium.com/itam/tagged/korean
/*
cleos wallet create -n goldbucket00
cleos wallet list
cleos wallet import -n goldbucket00 'your private key'
cleos wallet unlock -n goldbucket00 --password 'wallet passwd'

goldbucket01 - junglenet
Private Key: 
Public Key: 
wallet : 

goldbucket02 - junglenet
Private Key:  
Public Key: 
wallet : 

goldbucket03 - junglenet
Private Key:  
Public Key: 
wallet : 

goldbucket04 - junglenet
Private Key:  
Public Key: 
wallet : 

goldbucket00 - junglenet
Private Key:  
Public Key: 
wallet : 

goldbucket05 - junglenet
Private Key:  
Public Key: 
wallet : 

cleos wallet create -n goldbucket05 --to-console
cleos wallet import -n goldbucket05 --private-key 'your private key'
cleos wallet open -n goldbankgold

1. NETWORK INFO EDIT 기능 추가 필요 - 매번 삭제 / 등록해야 함.
2. NETWORK JUNGLENET ACCOUNT 등록후 REFRESH LINKED ACCOUNTS 해도 계정 찾지 못함.
3. tokens tab은 main net만 보여 주는지? concept인지?
4. SEND->SIMPLE 란의 SEND는 Mainnet만 가능한지?
*/

console.log('Hello world - eosjs_example');

const Eos = require('eosjs');

const config = {
    expireInSeconds: 60,
    broadcast: true,
    debug: false,
    sign: true,
    // mainNet bp endpoint
    httpEndpoint: 'https://api.eosnewyork.io',
    // mainNet chainId
    chainId: 'aca376f206b8fc25a6ed44dbdc66547c36c6c33e3a119ffbeaef943642f0e906',
};

// junglenet
const testnet_config = {
    expireInSeconds: 60,
    broadcast: true,
    debug: true,
    sign: true,
    httpEndpoint: 'http://jungle.cryptolions.io:18888',
    chainId: '038f4b0fc8ff18a4f0842a8f0564611f6e96e8535901dd45e43ac8691a1c4dca',
};



/*
const eos = Eos(config);

eos.getInfo((error, result) => { console.log(error, result) })

// Promise
eos.getAccount('goldbucket03')
    .then(result => console.log(result))
    .catch(error => console.error(error));

// callback
eos.getAccount('goldbucket03', (error, result) => console.log(error, result));

// Promise
eos.getBlock(14288098).then(result => console.log(result)).catch(error => console.error(error));

// Promise
eos.getCode('eosbetdice11', true)
    .then(result => console.log(result))
    .catch(error => console.error(error));

// Promise
eos.getAbi('eosbetdice11')
    .then(result => console.log(result))
    .catch(error => console.error(error));

// Promise
eos.getCurrencyBalance('eosblackteam', 'goldbucket03', 'BLACK')
    .then(result => console.log(result))
    .catch(error => console.error(error));

// Promise
eos.getCurrencyBalance('boidcomtoken', 'goldbucket05', 'BOID')
    .then(result => console.log(result))
    .catch(error => console.error(error));

// callback
eos.getCurrencyBalance('eosio.token', 'goldbucket05', 'EOS',
    (error, result) => console.log(error, result));

// Promise
eos.getKeyAccounts('EOS86uztN4fV19p4YinAvVDfJssc4qg8LF2EUV9dZmR95VMZVNQNo')
    .then(result => console.log(result))
    .catch(error => console.error(error));

// callback
eos.getKeyAccounts('EOS86uztN4fV19p4YinAvVDfJssc4qg8LF2EUV9dZmR95VMZVNQNo',
    (error, result) => console.log(error, result));

// Promise
eos.getCurrencyStats('eosio.token', 'EOS')
    .then(result => console.log(result))
    .catch(error => console.error(error));

// callback
eos.getCurrencyStats('boidcomtoken', 'BOID',
    (error, result) => console.log(error, result));


// https://api.eosnewyork.io/v1/chain/get_producers need to check later.
//eos.getProducers([json], lower_bound, [limit]);
//eos.getProducers(false, 0 , 50);
//eos.getProducerSchedule();

eos.getActions('goldbucket03', 1, 500)
    .then(result => console.log(result))
    .catch(error => console.error(error));

eos.getTransaction('6e3ff0d75193e2641b8b5107bced494e9bbd403ff6233ae2733af74d4bd5c3a1', 0)
//eos.getTransaction(6e3ff0d75193e2641b8b5107bced494e9bbd403ff6233ae2733af74d4bd5c3a1, block_num_hint)
    .then(result => console.log(result))
    .catch(error => console.error(error));

eos.getKeyAccounts('EOS86uztN4fV19p4YinAvVDfJssc4qg8LF2EUV9dZmR95VMZVNQNo')
    .then(result => console.log(result))
    .catch(error => console.error(error));

eos.getControlledAccounts('goldbucket05')
    .then(result => console.log(result))
    .catch(error => console.error(error));

eos.transfer();

// undelegate test
eos.undelegatebw({
    from: 'goldbucket00',
    receiver: 'goldbucket00',
    unstake_net_quantity: '50.0000 EOS',
    unstake_cpu_quantity: '50.0000 EOS',
    })
    .then(result => console.log(result))
    .catch(error => console.error(error));


let transactionOptions = {
    authorization: [ 'account@active`],
    broadcast: true,
    sign: true
};
eos.transaction(tr => {
    //tr.transfer(account.name, 'goldbucket04', '0.0001 EOS', 'transaction test memo1', transactionOptions);
    tr.transfer(account.name, 'goldbucket00', '0.0001 EOS', 'transaction test memo1');
    tr.transfer(account.name, 'goldbucket00', '0.0001 EOS', 'transaction test memo2');
    tr.transfer(account.name, 'goldbucket00', '0.0001 EOS', 'transaction test memo3');
    tr.transfer(account.name, 'goldbucket00', '0.0001 EOS', 'transaction test memo4');
    })
    .then(trx => {
        console.log(`Transaction ID: ${trx.transaction_id}`);
    })
    .catch(error => {
        console.error(error);
    });

*/


//==================================================================================
// delegate / undelegate test
//==================================================================================
/*
testnet_config.keyProvider = 'your private key';
stake_eos(Eos(testnet_config), "goldbucket01", "goldbucket01", '45.0000 EOS', '5.0000 EOS');
*/
function stake_eos(eos, from_account, to_account, cpu_quantity, net_quantity) {
    eos.delegatebw({
        from: from_account,
        receiver: to_account,
        stake_net_quantity: net_quantity,
        stake_cpu_quantity: cpu_quantity,
        transfer: 0
    })
}


/*
//goldbucket00
testnet_config.keyProvider = 'your private key';
unstake_eos(Eos(testnet_config), "goldbucket00", "goldbucket00", '49.0000 EOS', '49.0000 EOS');

// goldbucket03
testnet_config.keyProvider = ['your private key']
unstake_eos(Eos(testnet_config), "goldbucket03", "goldbucket03", '49.0000 EOS', '49.0000 EOS');

// goldbucket02
testnet_config.keyProvider = ['your private key']
unstake_eos(Eos(testnet_config), "goldbucket02", "goldbucket02", '99.0000 EOS', '99.0000 EOS');

// goldbucket04
testnet_config.keyProvider = ['your private key']
unstake_eos(Eos(testnet_config), "goldbucket04", "goldbucket04", '99.0000 EOS', '99.0000 EOS');
*/
function unstake_eos(eos, from_account, to_account, cpu_quantity, net_quantity) {
    eos.undelegatebw({
        from: from_account,
        receiver: to_account,
        unstake_net_quantity: net_quantity,
        unstake_cpu_quantity: cpu_quantity,
    })
    .then(result => console.log(result))
    .catch(error => console.error(error));
}



//==================================================================================
// Contract test
//==================================================================================
// set wast / abi code to contract
function set_contract(account_name, wastfile, abifile) {
    var wast = fs.readFileSync(wastfile);
    var abi = fs.readFileSync(abifile);

    // Publish contract to the blockchain
    eos.setcode(account_name, 0, 0, wast)
        .then(result => { console.log("WAST ok-"); console.log(result) })
        .catch(error => { console.log("WAST error-"); console.error(error) });

    eos.setabi(account_name, JSON.parse(abi))
        .then(result => { console.log("ABI ok-"); console.log(result) })
        .catch(error => { console.log("ABI error-"); console.error(error) });

}


// delete contract wast / abi code from account.
function delete_contract(account_name) {
    // remove contract
    eos.setcode(account_name, 0, 0, new Uint8Array())
        .then(result => { console.log("remove code ok-"); console.log(result) })
        .catch(error => { console.log("remove code error-"); console.error(error) });
    /*
    var abi = fs.readFileSync('./contract/null.abi');
    eos.setabi(account_name, JSON.parse(abi))
        .then(result => { console.log("remove abi ok-"); console.log(result) })
        .catch(error => { console.log("remove abi error-"); console.error(error) });
    */
}

// read wast / abi code from account
function check_contract(account_name) {
    // Promise
    eos.getCode(account_name, true)
        .then(result => console.log(result))
        .catch(error => console.error(error));


    // Promise
    eos.getAbi(account_name)
        .then(result => console.log(result))
        .catch(error => console.error(error));
}



const contract_account = 'goldbucket00';
const contract_classname = 'hello';
const user_account = 'goldbucket04';

//contract_test_hi(contract_account, contract_classname, user_account);

function contract_test_hi(contract_account, contract_classname, user_account) {
    // cleos -u https://api.eosnewyork.io push action contract_account hi user_account -p user_account@active
    eos.contract(contract_account)
        .then(contract_classname => {
            contract_classname.hi(user_account, { authorization: [user_account] })
                .then(res => {
                    console.log(res);
                });
        });
}


//==================================================================================
// Transfer / Transaction test
//==================================================================================

/*
testnet_config.keyProvider = ['your private key']
transfer_eos(Eos(testnet_config), "prizeswallet", "goldbucket01", '430.0000 EOS', 'transfer test........');
*/
function transfer_eos(eos, from, to, quantity, memo) {
    /*
    let transactionOptions = {
        authorization: [ from + '@active' ],
        broadcast: true,
        sign: true
    };
    */
    eos.transfer(from, to, quantity, memo)
    .then(trx => {
        console.log(`Transaction ID: ${trx.transaction_id}`);
    })
    .catch(error => {
        console.error(error);
    });
}

/*
testnet_config.keyProvider = ['your private key']
trxtransfer_eos(Eos(testnet_config), "goldbucket03", "----", '2.0000 EOS', 'transfer test........');
*/
function trxtransfer_eos(eos, from, to, quantity, memo) {
    eos.transaction(tr => {
        tr.transfer(from, 'goldbucket04', quantity, memo);
        tr.transfer(from, 'goldbucket02', quantity, memo);
    })
    .then(trx => {
            console.log(`Transaction ID: ${trx.transaction_id}`);
    })
    .catch(error => {
                console.error(error);
    });
}



//==================================================================================
// RAM Buy / Sell test
//==================================================================================
/*
testnet_config.keyProvider = ['your private key']
buyram_eosio(Eos(testnet_config), "goldbucket01", "goldbucket01", '35.0000 EOS');
*/
function buyram_eosio(eos, payer, reciever, quantity) {
    eos.buyram(payer, reciever, quantity)
        .then(result => console.log(result))
        .catch(error => console.error(error));
    
//    eos.buyrambytes({ payer: 'eosio', receiver: 'myaccount', bytes: 8192})
//        .then(result => console.log(result))
//        .catch(error => console.error(error));
}

/*
testnet_config.keyProvider = ['your private key']
sellram_eosio(Eos(testnet_config), "goldbucket03", 10*1024);
*/
function sellram_eosio(eos, seller, quantity) {
    eos.sellram(seller, quantity)
        .then(result => console.log(result))
        .catch(error => console.error(error));
}


//==================================================================================
// Create new account test
//==================================================================================
/*
testnet_config.keyProvider = ['your private key']
create_account(Eos(testnet_config), "goldbucket03", 'goldbucket01', 'EOS7gCuRUyNhBzNdDUF7VJWz9iMH7vfkWwokeGwZPLuZJJ9Vwe9Xe', 'EOS7gCuRUyNhBzNdDUF7VJWz9iMH7vfkWwokeGwZPLuZJJ9Vwe9Xe');
*/
function create_account(eos, creator_account, new_account, owner_pubkey, active_pubkey) {

    eos.transaction(tr => {
        tr.newaccount({
            creator: creator_account,
            name: new_account,
            owner: owner_pubkey,
            active: active_pubkey
        })

        tr.buyrambytes({
            payer: creator_account,
            receiver: new_account,
            bytes: 4 * 1024
        })

        tr.delegatebw({
            from: creator_account,
            receiver: new_account,
            stake_net_quantity: '0.1000 EOS',
            stake_cpu_quantity: '0.1000 EOS',
            transfer: 0
        })
    })
    .then(result => { console.log(result) })
    .catch(error => { console.error(error) });
}


/*
testnet_config.keyProvider = ['your private key']
get_table(Eos(testnet_config), "goldbucket01", 'goldbucket01', 'activebets', 'id');
*/
/*
eos.getTableRows({
        json: true,
        code:'CONTRACT_NAME',
        scope:'SCOPE_ACCOUNT (Normally contract)',
        table:'TABLE_NAME'
        table_key: 'school_key',
        lower_bound: school_id,
        })
*/

function get_table(eos, code, scope, table, tablekey) {
    // json, code, scope, table, table_key
    eos.getTableRows(true, code, scope, table, tablekey)
        .then((result) => { console.log(result) })
        .catch((error) => { colsole.error(error) });
}

//==================================================================================
// Create Token and issue / transfer test
//==================================================================================
//set contract eosio.token to account goldbucket01
//cleos -u http://jungle.cryptolions.io:18888 push action goldbucket01 create '["goldbucket01", "1000000000.0000 CON"]' -p goldbucket01

// issue
// cleos -u http://jungle.cryptolions.io:18888 push action goldbucket01 issue '["goldbucket04", "10000.0000 CON", "memo:thanks airdrop"]' -p goldbucket01
/*
executed transaction: 644d1a069c37fa20f804f00495f874c3ca505370c7dbf7ea224065162a923642  136 bytes  1270 us
#  goldbucket01 <= goldbucket01::issue          {"to":"goldbucket04","quantity":"10000.0000 CON","memo":"memo:thanks airdrop"}
#  goldbucket01 <= goldbucket01::transfer       {"from":"goldbucket01","to":"goldbucket04","quantity":"10000.0000 CON","memo":"memo:thanks airdrop"}
#  goldbucket04 <= goldbucket01::transfer       {"from":"goldbucket01","to":"goldbucket04","quantity":"10000.0000 CON","memo":"memo:thanks airdrop"}
*/



