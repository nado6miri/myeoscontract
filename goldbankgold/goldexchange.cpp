#include <utility>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>

#include <eosiolib/eosio.hpp>
#include <eosiolib/time.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/contract.hpp>
#include <eosiolib/crypto.h>
#include <eosiolib/action.h>
#include <eosiolib/transaction.hpp>
#include <boost/algorithm/string.hpp>


using namespace eosio;
using eosio::key256;
using eosio::indexed_by;
using eosio::const_mem_fun;
using eosio::asset;
using eosio::permission_level;
using eosio::action;
using eosio::print;
using eosio::name;

using eosio::unpack_action_data;
using eosio::symbol_type;
using eosio::transaction;
using eosio::time_point_sec;


class goldexchange : public eosio::contract {
	private:
		// taken from eosio.token.hpp
		struct st_transfer {
            account_name  from;
            account_name  to;
            asset         quantity;
            std::string   memo;
        };

		// taken from eosio.token.hpp
		struct account {
	    	asset    balance;
	    	uint64_t primary_key() const { return balance.symbol.name(); }
	    };
		typedef eosio::multi_index<N(accounts), account> accounts;

		std::string name_to_string(uint64_t acct_int) const
		{
			static const char* charmap = ".12345abcdefghijklmnopqrstuvwxyz";

			std::string str(13,'.');

		   	uint64_t tmp = acct_int;

		   	for( uint32_t i = 0; i <= 12; ++i )
			{
				char c = charmap[tmp & (i == 0 ? 0x0f : 0x1f)];
			  	str[12-i] = c;
			  	tmp >>= (i == 0 ? 4 : 5);
		   	}

		   	boost::algorithm::trim_right_if( str, []( char c ){ return c == '.'; } );
		   	return str;
		}

	public:
    using contract::contract;

		goldexchange(account_name self):eosio::contract(self)
		{}

		// @abi action
		void transfer(uint64_t sender, uint64_t receiver)
		{
		  const uint64_t MINBET = 1; // 0.0001 EOS
		  auto transfer_data = unpack_action_data<st_transfer>();
		  const uint64_t amount = (uint64_t)transfer_data.quantity.amount;
		  if(transfer_data.quantity.symbol == symbol_type(S(4, EOS)))
		  {
		  	eosio_assert( transfer_data.quantity.is_valid(), "Invalid asset");
		  	eosio_assert(MINBET < amount, "Must greater than 0 EOS");
		    eosio_assert(transfer_data.from != _self, "We don't accept incoming transfers from self!");
		    print("gold.token-transfer function called-recieved EOS-amount = ", amount, " from = ", name{sender}, " To = ", name{receiver});
		    // exchange
		    airdroptoken(sender, amount);
		  }
		}


		/*
		If you use userA permission to call action in contract A, and in the action you use inline_action_sender to call action in contract B,
		the contract B does not receive the userA permission. Instead it is use contractB's owner and permission "eosio.code" to call action in contractB.
		To allow contract B to use userA's permission, you can configure userA's permission to allow contractB owner and permission eosio.code to have
		userA's active permission.
		cleos set account permission userA active '{"threshold":1, "keys":[{"key":"...", "weight":1}],
		"accounts": [{"permission":{"actor":"contractBuser","permission":"eosio.code"},"weight":1}]}' owner -p userA

		We enhanced the security under Dawn 4.0, you need to set the permission level "kc.code@eosio.code" rather than "kc.code@active".
		Actions dispatched by code are no longer dispatched with "active" authority, but with a special "eosio.code" authority level.
		cleos set account permission mk active '{"threshold": 1,"keys": [{"key": "EOS7ijWCBmoXBi3CgtK7DJxentZZeTkeUnaSDvyro9dq7Sd1C3dC4","weight": 1}],
												"accounts": [{"permission":{"actor":"ck.code","permission":"eosio.code"},"weight":1}]}' owner -p mk
		*/
		//cleos -u http://jungle.cryptolions.io:18888 set account permission goldexchange active '{"threshold": 1,"keys": [{"key": "EOS5UJkjUvJZnuU4zrtdpMhGfxpW2Kk3CK9NGgLi2kvHmbcnpSEVj","weight": 1}],"accounts": [{"permission":{"actor":"goldexchange","permission":"eosio.code"},"weight":1}]}' owner -p goldexchange
		/*
		cleos -u http://jungle.cryptolions.io:18888 push action eosio.token transfer '["goldmonet001", "goldexchange", "1.0000 EOS", "Exchange test..."]' -p goldmonet001
		executed transaction: 75740d0138f1a013f6499f9547956e8ede594df59289e88790d42cea411c9b51  144 bytes  1944 us
		#   eosio.token <= eosio.token::transfer        {"from":"goldmonet001","to":"goldexchange","quantity":"1.0000 EOS","memo":"Exchange test..."}
		#  goldmonet001 <= eosio.token::transfer        {"from":"goldmonet001","to":"goldexchange","quantity":"1.0000 EOS","memo":"Exchange test..."}
		#  goldexchange <= eosio.token::transfer        {"from":"goldmonet001","to":"goldexchange","quantity":"1.0000 EOS","memo":"Exchange test..."}
		>> gold.token-transfer function called-recieved EOS-amount = 10000 from = goldmonet001 To = goldexchange
		#  goldtoken000 <= goldtoken000::transfer       {"from":"goldexchange","to":"goldmonet001","quantity":"1.0000 GOLD","memo":"exchange EOS to GOLD to...
		#  goldexchange <= goldtoken000::transfer       {"from":"goldexchange","to":"goldmonet001","quantity":"1.0000 GOLD","memo":"exchange EOS to GOLD to...
		#  goldmonet001 <= goldtoken000::transfer       {"from":"goldexchange","to":"goldmonet001","quantity":"1.0000 GOLD","memo":"exchange EOS to GOLD to...
		warning: transaction executed locally, but may not be confirmed by the network yet
		*/
		void airdroptoken(const uint64_t sender, const uint64_t amount)
		{
		  const float ratio = 1;
			const uint64_t token_balance = get_token_balance(N(goldtoken000), symbol_type(S(4, GOLD)));

			// no need to require auth....
			//require_auth(_self); or require_auth2(_self, N(active));

		  // 1:1 ==> 0.0001 EOS - 0.0001 Gold --> amount = 1
		  // 1:1 ==> 1.0000 EOS - 1.0000 EOS --> amount = 10000
			uint64_t drop_amt = amount * ratio;
			print("\nbalance = ", token_balance, " drop_amount = ", drop_amt);
		  eosio_assert((token_balance >= drop_amt/10000), "\n[Error] Insufficient goldtoken balance!!!");

			action(
		           permission_level{_self, N(active)},
		           N(goldtoken000),
		           N(transfer),
		           std::make_tuple(
		               _self,
		               name{sender},
		               asset(drop_amt, symbol_type(S(4, GOLD))),
									 // exchange EOS to GOLD token -- 7287555726296854496 -- Enjoy airdrop! Contribution : donate.io
		               std::string("exchange EOS to GOLD token -- ") + std::to_string(name{sender}) + std::string(" -- Enjoy airdrop! Contribution : donate.io")
		          )
		     ).send();
		}


		uint64_t get_token_balance(const account_name token_contract, const symbol_type& token_type) const
		{
			accounts from_accounts(token_contract, _self);

			const auto token_name = token_type.name();
			auto my_account_itr = from_accounts.find(token_name);
			if (my_account_itr == from_accounts.end()){
				return 0;
			}
			const asset my_balance = my_account_itr->balance;
			return (uint64_t)my_balance.amount;
		}
};

// EOSIO_ABI_EX를 추가한 이유는 eosio.token의 transfer 함수에서 require_recipient( from );, require_recipient( to ); contract에서 notification 을 받아 처리할 수 있도록 함.
// require_recipient 기능을 이용하면 입금 기능말고 출금을 자동으로 로깅하게 할 수 있습니다. 컨트랙트에서 출금이 발생했을 때 지출 내역이라면 해당 내용을 기록 하는 것입니다.
// https://steemit.com/eos/@raindays/contract
// https://steemit.com/eos/@raindays/7uqisg-eos-knights-transfer-hack-statement
#define EOSIO_ABI_EX( TYPE, MEMBERS ) \
extern "C" { \
   void apply( uint64_t receiver, uint64_t code, uint64_t action ) { \
      auto self = receiver; \
      if( code == self || code == N(eosio.token)) { \
      	 if( action == N(transfer)){ \
      	 	eosio_assert( code == N(eosio.token), "Must transfer EOS"); \
      	 } \
         TYPE thiscontract( self ); \
         switch( action ) { \
            EOSIO_API( TYPE, MEMBERS ) \
         } \
         /* does not allow destructor of thiscontract to run: eosio_exit(0); */ \
      } \
   } \
}

EOSIO_ABI_EX(goldexchange, (transfer) (airdroptoken))

// 1. EOS가 아닌 GOLD Token으로 전송시.... transfer 처리.... 어떻게 ?? token symbol / contract 비교 필요...

// 15. Exchage Test (EOS --> GOLD TOKEN)
// cleos -u http://jungle.cryptolions.io:18888 push action eosio.token transfer '["goldmonet001", "goldexchange", "1.0000 EOS", "Exchange test..."]' -p goldmonet001
