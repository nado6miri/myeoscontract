'use strict';

// https://github.com/EOSIO/eosjs
// https://developers.eos.io/
// https://www.eosdocs.io/resources/apiendpoints/
// https://medium.com/itam/tagged/korean

/*
mercedezbenz - junglenet
Private Key: 'your private key'
*/

console.log('eosjs set/get/delete contract');

// set smart contract...
const config = {
    expireInSeconds: 60,
    broadcast: true,
    debug: true,
    sign: true,
    httpEndpoint: 'https://api.eosnewyork.io',
    //httpEndpoint: 'https://bp.libertyblock.io:8890',
    chainId: 'aca376f206b8fc25a6ed44dbdc66547c36c6c33e3a119ffbeaef943642f0e906',
};
config.binaryen = require("binaryen");

// junglenet
const testnet_config = {
    expireInSeconds: 60,
    broadcast: true,
    debug: true,
    sign: true,
    httpEndpoint: 'http://jungle.cryptolions.io:18888',
    chainId: '038f4b0fc8ff18a4f0842a8f0564611f6e96e8535901dd45e43ac8691a1c4dca',
    keyProvider: 'your private key', // mercedezbenz - token issuer
};
	
testnet_config.binaryen = require("binaryen");


const fs = require("fs");
const Eos = require("eosjs");
const eos = Eos(testnet_config);
//const eos = Eos(config);



var wastfile = './gold.token.wast';
var abifile = './gold.token.abi';
set_contract('mercedezbenz', wastfile, abifile);
//delete_contract('mercedezbenz');
//check_contract('mercedezbenz');


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

function check_contract(account_name) {
    eos.contract(account_name)
        .then(hello => {
            hello.hi('axay', { authorization: ["account_name"] })
                .then(res => {
                    //currently taking username static 'axay'
                    console.log(res);
                });
        });
}


function delete_contract(account_name) {
    // remove contract
    eos.setcode(account_name, 0, 0, new Uint8Array())
        .then(result => { console.log("remove code ok-"); console.log(result) })
        .catch(error => { console.log("remove code error-");console.error(error) });
    /*
    var abi = fs.readFileSync('./contract/null.abi');
    eos.setabi(account_name, JSON.parse(abi))
        .then(result => { console.log("remove abi ok-"); console.log(result) })
        .catch(error => { console.log("remove abi error-"); console.error(error) });
    */
}


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

