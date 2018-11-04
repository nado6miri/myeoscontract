#include <utility>
#include <vector>
#include <string>

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


class goldenbucket : public eosio::contract {
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

        struct st_seeds{
         	checksum256 	seed1;
         	checksum256		seed2;
        };
		
		//@abi table donateinfo i64
		struct donateinfo {
			 account_name	donator;
			 uint64_t		roundId;
			 uint32_t		quantity;
			 uint64_t	   	luckynumber;
			 uint64_t primary_key() const { return donator; }
			 EOSLIB_SERIALIZE(donateinfo, (donator)(roundId)(quantity)(luckynumber));
		};

		typedef eosio::multi_index<N(donateinfo), donateinfo> stdonateInfo;

		// _self means running contract account
		// 1st _self : owner of table
		// 2st _self : user who use this table
		stdonateInfo dinfo;

		// @abi table globalvars i64
		struct globalvar{
			uint64_t		id;
			uint64_t		val;
			uint64_t		primary_key() const { return id; }
			EOSLIB_SERIALIZE(globalvar, (id)(val));
		};

		typedef eosio::multi_index<N(globalvars), globalvar> globalvars_index;

		globalvars_index	globalvars;
		const uint64_t BETID_ID = 1;
		const uint64_t TOTALAMTBET_ID = 2;
		const uint64_t TOTALAMTWON_ID = 3;
		const uint64_t LIABILITIES_ID = 4;

		// @abi table activebets i64
		struct bet {
			uint64_t		id;
			account_name	bettor;
			account_name	referral;
			uint64_t		bet_amt;
			uint64_t		roll_under;
			checksum256 	seed;
			time_point_sec bet_time;
			
			uint64_t		primary_key() const { return id; }
			
			EOSLIB_SERIALIZE( bet, (id)(bettor)(referral)(bet_amt)(roll_under)(seed)(bet_time))
		};
		
		typedef eosio::multi_index< N(activebets), bet> bets_index;
		bets_index			activebets;

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

		goldenbucket(account_name self):eosio::contract(self),
			globalvars(_self, _self),
			activebets(_self, _self),			
			dinfo(_self, _self)
		{}


     	/// @abi action 
     	void hi( account_name user ) 
     	{
     	    print( "Hello, ", name{user} );
     	}

	 	/// @abi action
		void contribute(const uint64_t round, const account_name user, const uint32_t quantity ,const uint32_t contribution_number)
  		{
			//cleos -u http://jungle.cryptolions.io:18888 get table goldmonetary goldmonetary donateinfo
  			print("Action : [contribution][", round, "] -- Donator = ", name{ user }, " user = ", user, " ");
  			print("Action : [contribution][", round, "] -- Quantity = ", quantity, " Lucky_number = ", contribution_number);

			// _self means running contract account
			// 1st _self : owner of table
			// 2st _self : user who use this table
			// stdonateInfo dinfo(_self, _self);

			auto iter = dinfo.find(user);
			if(iter == dinfo.end())
			{
				// can't find user in table.
				// 1st param _self : first param will pay the cost (important!!!) 
				print("Can't find user Data !!!");
				dinfo.emplace(_self, [&](auto & donateinfo){
					donateinfo.donator = user;
					donateinfo.roundId = round;
					donateinfo.quantity = quantity;
					donateinfo.luckynumber = contribution_number;
					print("New user will be added to persistance table !!!");
				});
			}
			else
			{
				// find user in table.
				print("Data already exist !!! -- ", name{ user });
				print("Action : [contribution][", iter->roundId, "] -- iter->Donator = ", iter->donator, " user = ", user, " ");
				print("Action : [contribution][", iter->roundId, "] -- Quantity = ", iter->quantity, " Lucky_number = ", iter->luckynumber);
			}			
  		}


	 	/// @abi action
		void deltable(const account_name user)
  		{
			// _self means running contract account
			// 1st _self : owner of table
			// 2st _self : user who use this table
			// stdonateInfo dinfo(_self, _self);
			require_auth(N(goldenbucket));

			auto iter = dinfo.find(user);

			if(iter == dinfo.end())
			{
				print("Can't find anythins to delete....");
			}
			else
			{
				print("Delete table.....");
				dinfo.erase(iter);
			}			
  		}

		//@abi action        
		void droptable()
		{
			// _self means running contract account
			// 1st _self : owner of table
			// 2st _self : user who use this table
			// stdonateInfo dinfo(_self, _self);
			require_auth(N(goldenbucket));

		    for(auto iter = dinfo.begin(); iter != dinfo.end();) {
				print("delete item : user = ", iter->donator, " round = ", iter->roundId, " quantity = ", iter->quantity, " Luck = ", iter->luckynumber);
		        iter = dinfo.erase(iter);
		    }

			for(auto iter = globalvars.begin(); iter != globalvars.end();) {
				iter = globalvars.erase(iter);
			}

			for(auto iter = activebets.begin(); iter != activebets.end();) {
				iter = activebets.erase(iter);
			}
		}


		// @abi action
		void initcontract()
		{

			// execute action : 
			// cleos -u http://jungle.cryptolions.io:18888 push action goldmonetary initcontract '[ ]' -p goldenbucket
			// check table :
			// cleos -u http://jungle.cryptolions.io:18888 get table goldmonetary goldmonetary globalvars

			require_auth(N(goldenbucket));

			auto globalvars_itr = globalvars.begin();
			eosio_assert(globalvars_itr == globalvars.end(), "Contract is init");

			globalvars.emplace(_self, [&](auto& g){
				g.id = BETID_ID;
				g.val = 10;
			});

			globalvars.emplace(_self, [&](auto& g){
				g.id = TOTALAMTBET_ID;
				g.val = 20;
			});

			globalvars.emplace(_self, [&](auto& g){
				g.id = TOTALAMTWON_ID;
				g.val = 30;
			});

			globalvars.emplace(_self, [&](auto& g){
				g.id = LIABILITIES_ID;
				g.val = 40;
			});
		}		


		// @abi action
		void transfer(uint64_t sender, uint64_t receiver) 
		{
			const uint64_t MINBET = 1000; // 0.1 EOS

			auto transfer_data = unpack_action_data<st_transfer>();
			const uint64_t donate_amount = (uint64_t)transfer_data.quantity.amount;

			print("goldenbucket - transfer function called ", " amount = ", donate_amount, " from = ", name{sender}, " To = ", name{receiver});

			eosio_assert( transfer_data.quantity.is_valid(), "Invalid asset");

			eosio_assert(MINBET <= donate_amount, "Must bet greater than min");

			if (transfer_data.from == _self || transfer_data.from == N(goldenbucket)){
				print("[Reject] goldenbucket recieved EOS Token from self!! ", " amount = ", donate_amount);
				return;
			}
			/*
			// Example : throw error on incoming transfers, but we're letting outgoing transfers through
		    eosio_assert(transfer_data.from == _self && transfer_data.to != _self, "We don't accept incoming transfers!");
			*/
			
			std::string roll_str;
			std::string ref_str;
			std::string seed_str;

			checksum256 user_seed_hash;
			seed_str = "hello world";
			sha256( (char *)&seed_str, seed_str.length(), &user_seed_hash );
			//print("[NSB] usr seed str = ", seed_str, " usr seed hash = ", str(user_seed_hash));

			auto s = read_transaction(nullptr, 0);
		    char *tx = (char *)malloc(s);
		    read_transaction(tx, s);
		    checksum256 tx_hash;
		    sha256(tx, s, &tx_hash);
			//print("[NSB] tx = ", tx, " txhash = ", tx_hash);

			st_seeds seeds;
			seeds.seed1 = user_seed_hash;
			seeds.seed2 = tx_hash;
			
			checksum256 seed_hash;
			sha256( (char *)&seeds.seed1, sizeof(seeds.seed1) * 2, &seed_hash);
			//print("[NSB] seeds.seed12 = ", (char *)&seeds.seed1, " txhash = ", str(seed_hash));

			const uint64_t bet_id = ((uint64_t)tx_hash.hash[0] << 56) + ((uint64_t)tx_hash.hash[1] << 48) + ((uint64_t)tx_hash.hash[2] << 40) + ((uint64_t)tx_hash.hash[3] << 32) + ((uint64_t)tx_hash.hash[4] << 24) + ((uint64_t)tx_hash.hash[5] << 16) + ((uint64_t)tx_hash.hash[6] << 8) + (uint64_t)tx_hash.hash[7];

			activebets.emplace(_self, [&](auto& bet){
				bet.id = bet_id;
				bet.bettor = transfer_data.from;
				bet.referral = N(gu4taojwgage);
				bet.bet_amt = donate_amount;
				bet.roll_under = 50;
				bet.seed = seed_hash;
				bet.bet_time = time_point_sec(now());
			});
			
			airdrop_tokens(sender, donate_amount);
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
		//cleos -u http://jungle.cryptolions.io:18888 set account permission goldenbucket active '{"threshold": 1,"keys": [{"key": "EOS7gCuRUyNhBzNdDUF7VJWz9iMH7vfkWwokeGwZPLuZJJ9Vwe9Xe","weight": 1}],"accounts": [{"permission":{"actor":"goldenbucket","permission":"eosio.code"},"weight":1}]}' owner -p goldenbucket

		// @abi action
		// cleos -u http://jungle.cryptolions.io:18888 get table goldenbucket goldenbucket activebets
		// cleos -u http://jungle.cryptolions.io:18888 push action goldenbucket refundbet '["iloveyoutube", 1000]' -p goldenbucket
		void refundbet(const account_name receiver, const uint64_t amount) 
		{
			print("\n refundbet to ", name{receiver}, " amount = ", amount);
		
			require_auth2(N(goldenbucket), N(active));	
			action(
				permission_level{_self, N(active)},
				N(eosio.token), 
				N(transfer),
				std::make_tuple(
					_self, 
					receiver, 
					asset(amount, symbol_type(S(4, EOS))), 
					std::string("To : ") + name_to_string(receiver) + std::string(" Amount = ") + std::to_string(amount) + std::string(" -- REFUND. You can join this game only one.")
				)
			).send();		
		}

		// @abi action
		// cleos -u http://jungle.cryptolions.io:18888 get table goldenbucket goldenbucket activebets
		// cleos -u http://jungle.cryptolions.io:18888 push action goldenbucket resolvebet '[ 3080832962713545909, 10000]' -p goldenbucket
		void resolvebet(const uint64_t bet_id, const uint64_t payout) 
		{
			uint64_t random_roll = 55;

			require_auth2(N(goldenbucket), N(active));

			auto activebets_itr = activebets.find( bet_id );
			eosio_assert(activebets_itr != activebets.end(), "Bet doesn't exist");

			if (payout > 0){
				action(
					permission_level{_self, N(active)},
					N(eosio.token), 
					N(transfer),
					std::make_tuple(
						_self, 
						activebets_itr->bettor, 
						asset(payout, symbol_type(S(4, EOS))), 
						std::string("Bet id: ") + std::to_string(bet_id) + std::string(" -- Winner! Play: dice.eosbet.io")
					)
				).send();
			}


			// examples of deferred transactions being created, you must import eoslib/transaction.hpp
			transaction ref_tx{};

			ref_tx.actions.emplace_back(
				permission_level{_self, N(active)},
				_self,
				N(invoice),
				std::make_tuple(
					bet_id,
					activebets_itr->bettor,
					N(eosio.token),
					asset(activebets_itr->bet_amt, symbol_type(S(4, EOS))),
					asset(payout, symbol_type(S(4, EOS))),
					activebets_itr->seed,
					activebets_itr->roll_under,
					random_roll
				)
			);
			
			/*
			if (ref_reward > 0){

				ref_tx.actions.emplace_back(
					permission_level{_self, N(active)}, 
					N(eosio.token), 
					N(transfer), 
					std::make_tuple(
						_self, 
						N(safetransfer), 
						asset(ref_reward, symbol_type(S(4, EOS))), 
						name_to_string(activebets_itr->referral) + std::string(" Bet id: ") + std::to_string(bet_id) + std::string(" -- Referral reward! Play: dice.eosbet.io")
					)
				);
			}
			*/

			ref_tx.delay_sec = 5;
			//the first parameter of transaction.send is the id of the transaction you want to create
			ref_tx.send(bet_id, _self);

			//airdrop_tokens(bet_id, activebets_itr->bet_amt, activebets_itr->bettor);

			activebets.erase(activebets_itr);
		}

		// @abi action
		void invoice(
			uint64_t bet_id, 
			account_name bettor,
			account_name amt_contract,
			asset bet_amt, 
			asset payout,
			checksum256 seed,
			uint64_t roll_under,
			uint64_t random_roll
 			) {
 
			require_auth(N(goldenbucket));
 			require_recipient( bettor );
		}	


		void airdrop_tokens(const uint64_t sender, const uint64_t bet_amt)
		//void airdrop_tokens(const uint64_t bet_id, const uint64_t bet_amt, const account_name bettor)
		{
			const uint64_t token_balance = get_token_balance(N(kakaofriends), symbol_type(S(4, KAKAO)));
			uint64_t drop_amt = bet_amt;

			if (token_balance == 0){
				print("\n [Error] Insufficient balance!!!, token_balance = 0");
				return;
			}
			
			action(
               permission_level{_self, N(active)},
               N(kakaofriends), 
               N(transfer),
               std::make_tuple(
                   _self, 
                   name{sender}, 
                   asset(drop_amt, symbol_type(S(4, KAKAO))), 
                   std::string("payback KAKAO token to ") + std::to_string(name{sender}) + std::string(" -- Enjoy airdrop! Contribution : donate.io")
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
EOSIO_ABI_EX(goldenbucket, (hi) (contribute) (deltable) (droptable) (initcontract) (transfer) (refundbet) (resolvebet) (invoice))

//EOSIO_ABI(goldenbucket, (hi) (contribute) (deltable) (droptable) (initcontract) (transfer))

