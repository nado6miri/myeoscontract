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
#include <eosiolib/singleton.hpp>

#include <string>
#include <map>
#include <stdlib.h>

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

using std::string;

enum ENUM_INFOTYPE {
	LUKCY_NUMBER = 0,
	ROUND_TYPE = 1,
	ENUM_MAX = ROUND_TYPE
};

enum ENUM_ROUND_INFO {
	ROUND_TYPE0 = 0,
	ROUND_TYPE1 = 1,
	ROUND_TYPE2 = 2,
	ROUND_TYPE3 = 3,
	ROUND_TYPE4 = 4,
	ROUND_TYPE_MAX = ROUND_TYPE4+1,
};
/*
typedef struct roundinfo {
	ENUM_ROUND_INFO roundtype;
	uint32_t	max_count;
}stRoundInfo;

const stRoundInfo round_infomation[5] = {
	{ ROUND_TYPE0, 100 },
	{ ROUND_TYPE1, 500 },
	{ ROUND_TYPE2, 1000 },
	{ ROUND_TYPE3, 5000 },
	{ ROUND_TYPE4, 10000 },
};
*/
#define MAX_DONATE_COUNT 10

#define ENABLE_MULTI_ROUND	true

class goldenbucket : public eosio::contract {
	private:
		const uint64_t MINBET = 10000; // 1 EOS
		const uint64_t MINBET_KAKAO = 10000*100; // 100 KAKAO
		
		// taken from eosio.token.hpp
		struct st_transfer {
            account_name  from;
            account_name  to;
            asset         quantity;
            std::string   memo;
        };

		// taken from eosio.token.hpp
		struct st_account {
	    	asset    balance;
	    	uint64_t primary_key() const { return balance.symbol.name(); }
	    };
		typedef eosio::multi_index<N(accounts), st_account> accounts;


		// cleos -u http://jungle.cryptolions.io:18888 get table -l 100 goldenbucket goldenbucket winnerlists
		// @abi table winnerlists i64
		struct st_winlist {
			uint64_t		round;		// round no : sequential number 0, 1, 2, ... N
			uint8_t			roundtype;	// 0 : 100, 1: 500, 2: 1000, 3: 5000, 4: 10000
			time_point_sec 	jointime;	// joint time
			account_name	beneficiery;// winner's account name
			uint32_t		luckycode;	// winner's luckycode
			uint64_t		eos_amt;	// EOS Awards 
			uint64_t		gold_amt;	// GOLD Awards
			bool			eos_payout;		// Y : already sent award to winner, N : Need to send token to winner.
			bool			gold_payout;		// Y : already sent award to winner, N : Need to send token to winner.
			uint64_t		primary_key() const { return round; }

			EOSLIB_SERIALIZE(st_winlist, (round)(roundtype)(jointime)(beneficiery)(luckycode)(eos_amt)(gold_amt)(eos_payout)(gold_payout))
		};

		
		typedef eosio::multi_index< N(winnerlists), st_winlist> winnerlists;
		winnerlists	winner_list;


		// cleos -u http://jungle.cryptolions.io:18888 get table goldenbucket goldenbucket donateinfos
		// cleos -u http://jungle.cryptolions.io:18888 get table goldenbucket goldenbucket roundinfos
		// @abi table roundinfos i64
		struct st_roundinfo {
			uint64_t		id;	// tx info
			time_point_sec 	jointime;
			uint64_t		roundid;
			account_name	donator;
			account_name	referral;
			uint64_t		donateamt;
			uint32_t		luckycode;
			uint8_t			donatetype; // 0 : EOS, 1 : Gold Token ....

			uint64_t		primary_key() const { return id; }

			EOSLIB_SERIALIZE(st_roundinfo, (id)(jointime)(roundid)(donator)(referral)(donateamt)(luckycode)(donatetype))
		};

		
		typedef eosio::multi_index< N(roundinfos), st_roundinfo> round_infotable;
		round_infotable	round_info;

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

		// @abi table stbucketcfgs i64
		// cleos -u http://jungle.cryptolions.io:18888 get table goldenbucket goldenbucket stbucketcfgs
		struct stbucketcfg {
			bool				 contract_enable;	
			bool				 is_closeround_transfer;	
			bool				 round0_runnable;	
			bool				 round1_runnable;	
			bool				 round2_runnable;	
			bool				 round3_runnable;	
			bool				 round4_runnable;	
			uint64_t			 create_time;
			vector<account_name> blacklist;
		};
		
		// sigletone has only one instance and serve global access.
		typedef singleton<N(stbucketcfgs), stbucketcfg> bucketcfg_singleton;
		
		bucketcfg_singleton		bucket_cfg;


	public:
    	using contract::contract;
		
		// _self means running contract account
		// 1st _self : owner of table
		// 2st _self : user who use this table
		// donateInfo round_info(_self, _self);
		goldenbucket(account_name self):eosio::contract(self),
			bucket_cfg(_self, _self),
			round_info(_self, _self),
			winner_list(_self, _self)
		{}


		/*
		void delitemfromtable(round_infotable &rt, const uint64_t id)
		{
			auto iter = rt.find(id);
			
			if(iter == rt.end()) { print("Can't find anythins to delete...."); }
			else { print("Delete table....."); rt.erase(iter); }
		}
		*/

		/// @abi action
		// cleos -u http://jungle.cryptolions.io:18888 push action goldenbucket deltable '["round0", "15657998322051070431"]' -p goldenbucket
		void deltable(const string    del_table, const uint64_t id)
  		{
			eosio_assert(is_contract_freezed() == false, "contract is frozen");
			#define DEL_ITEM_FROM_TABLE(TABLE_NAME, ID) \
			{\
				auto iter = TABLE_NAME.find(ID);\
				if(iter == TABLE_NAME.end()) { print("Can't find anythins to delete...."); }\
				else { print("Delete table....."); TABLE_NAME.erase(iter); }\
			};
				
			require_auth(_self);
			// _self means running contract account
			if(del_table.compare("round") == 0){ DEL_ITEM_FROM_TABLE(round_info, id); }
			else if(del_table.compare("winnerlists") == 0){ DEL_ITEM_FROM_TABLE(winner_list, N(id)); }
			else { eosio_assert(false, " Undefined Roundl-Can't delete/drop table..."); }	
  		}

		/*
		void del_alldata_from_table(round_infotable &rt)
		{
			for(auto iter = rt.begin(); iter != rt.end();) { iter = rt.erase(iter); }
		}
		*/

		// cleos -u http://jungle.cryptolions.io:18888 push action goldenbucket droptable '["round0"]' -p goldenbucket
		//@abi action
		void droptable(const string    del_table)
		{
			eosio_assert(is_contract_freezed() == false, "contract is frozen");
			#define DROP_ALLITEM_FROM_TABLE(TABLE_NAME) \
			{\
				for(auto iter = TABLE_NAME.begin(); iter != TABLE_NAME.end();) { iter = TABLE_NAME.erase(iter); }\
			};

			require_auth(_self);
			// _self means running contract account
			// delete signleton
			if(del_table.compare("stbucketcfgs") == 0) 
			{ 
				if (bucket_cfg.exists())
				{
					auto bc = stbucketcfg{};
					bucket_cfg.remove(); 
					//bucket_cfg.set(bc, _self);
				}
				return; 
			}
			
			if(del_table.compare("round") == 0) { DROP_ALLITEM_FROM_TABLE(round_info); }
			else if(del_table.compare("winnerlists") == 0) { DROP_ALLITEM_FROM_TABLE(winner_list); }
			else { eosio_assert(false, " Undefined Round-Can't delete/drop table..."); }	
		}


		// @abi action
		// cleos -u http://jungle.cryptolions.io:18888 push action eosio.token transfer '["goldmonetary", "goldenbucket", "1.0000 EOS", "Contribution-1-100"]' -p goldmonetary
		// cleos -u http://jungle.cryptolions.io:18888 push action kakaofriends transfer '["ilovelgtwins", "goldenbucket", "100.0000 KAKAO", "Contribution-1-1000"]' -p ilovelgtwins
		void transfer(uint64_t sender, uint64_t receiver)
		{
			uint8_t type = 0;
			auto transfer_data = unpack_action_data<st_transfer>();
			const uint64_t donate_amount = (uint64_t)transfer_data.quantity.amount;

			eosio_assert(is_contract_freezed() == false, "contract is frozen");
			print("goldenbucket:transfer", "-amount = ", donate_amount, " sender = ", name{sender}, " reciever = ", name{receiver});
			print("\ngoldenbucket:transfer", "-amount = ", donate_amount, " from = ", name{transfer_data.from}, " To = ", name{transfer_data.to});

			//if(is_closeroundtransfer() == true) { return; }
			
			// why ?? goldenbucket:transfer-amount = 900000 from = goldbankgold To = goldenbucket - because of contract@eosio.code settings ??
			if (transfer_data.from == _self || transfer_data.from == N(goldenbucket)){
				print(" [Reject] goldenbucket transfer Token to another!! don't save table, accept transfer action!");
				return;
			}

			if(is_contribution_transfer(transfer_data.memo) == false) // normal transfer
			{
				print(" accept Normal transfer action!");
				return;
			}
			else // contribution case..
			{
				// after selecting beneficiary... when contract transfers award to beneficiary, 
				// this transfer function will be called... don't add table and accept transfer...
				eosio_assert(is_roundenable(ROUND_TYPE0), "round is full(disable)... Please select beneficiary");

				uint32_t luckynumber = get_userluckycode(transfer_data.memo);
				uint32_t roundtype = get_joinround(transfer_data.memo);
				print(" lucky number = ", luckynumber, " roundtype = ", roundtype);

				eosio_assert(transfer_data.quantity.is_valid(), "Invalid asset");
				
				if(transfer_data.quantity.symbol == symbol_type(S(4, EOS)))
				{
					type = 0;
					print(" Token = EOS ");
					eosio_assert(MINBET == donate_amount, "Must donate 1 EOS");
				}
				else if(transfer_data.quantity.symbol == symbol_type(S(4, KAKAO)))
				{
					type = 1;
					print(" Token = KAKAO ");
					eosio_assert(MINBET_KAKAO == donate_amount, "Must donate 100 KAKAO");
				}
				else
				{
					eosio_assert(false, " Undefined Symbol!!!");
				}

				// how to get transaction ID
				// checksum256 is a struct containing an array of 32 uint8 numbers.
				auto size = transaction_size();
				auto s = read_transaction(nullptr, 0);
			    char *tx = (char *)malloc(s);
			    auto read_size = read_transaction(tx, s);
				eosio_assert(size == read_size, "read_transaction failed");
			    checksum256 tx_hash;
			    sha256(tx, read_size, &tx_hash);
				printhex(&tx_hash, sizeof(tx_hash));

				const uint64_t donate_id = ((uint64_t)tx_hash.hash[0] << 56) + ((uint64_t)tx_hash.hash[1] << 48) + ((uint64_t)tx_hash.hash[2] << 40) + ((uint64_t)tx_hash.hash[3] << 32) + ((uint64_t)tx_hash.hash[4] << 24) + ((uint64_t)tx_hash.hash[5] << 16) + ((uint64_t)tx_hash.hash[6] << 8) + (uint64_t)tx_hash.hash[7];
				add_donation_info(round_info, donate_id, transfer_data.from, donate_amount, luckynumber, type); 
			}
		}

		void add_donation_info(round_infotable &rt, const uint64_t donate_id, const account_name from, 
									const uint64_t donate_amount, const uint32_t luckynumber, const uint8_t type)
		{
			uint64_t index = 0;

			// find max round id
			for(auto iter = rt.begin(); iter != rt.end(); iter++)
			{
				if(index < iter->roundid) { index = iter->roundid; }
			}
			index++;

			rt.emplace(_self, [&](auto& st_roundinfo){
				st_roundinfo.id = donate_id;						// unique number
				st_roundinfo.jointime = time_point_sec(now());	// join time
				st_roundinfo.donator = from;		// donator
				st_roundinfo.roundid = index;						// sequential number
				st_roundinfo.referral = N(goldenbucket);			// referral id
				st_roundinfo.donateamt = donate_amount; 		// contribution amount (EOS or GOLD)
				st_roundinfo.luckycode = luckynumber;				// user selected number
				st_roundinfo.donatetype = type; 					// EOS or GOLD
			});

			if(index >= MAX_DONATE_COUNT)
			{
				print("\nSelect Beneficiary!!!!", " index = ", index);
				round_control(0, false);
				//eosio_assert(false, " index >= MAX_DONATE_COUNT!!!");
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
		//cleos -u http://jungle.cryptolions.io:18888 set account permission goldenbucket active '{"threshold": 1,"keys": [{"key": "EOS7gCuRUyNhBzNdDUF7VJWz9iMH7vfkWwokeGwZPLuZJJ9Vwe9Xe","weight": 1}],"accounts": [{"permission":{"actor":"goldenbucket","permission":"eosio.code"},"weight":1}]}' owner -p goldenbucket

		// @abi action
		// cleos -u http://jungle.cryptolions.io:18888 push action goldenbucket refunddonate '["goldmonetary", "1.0000 EOS"]' -p goldenbucket
		// cleos -u http://jungle.cryptolions.io:18888 push action goldenbucket refunddonate '["goldmonetary", "100.0000 KAKAO"]' -p goldenbucket
		void refunddonate(account_name receiver, asset quantity)
		{
			eosio_assert(is_contract_freezed() == false, "contract is frozen");
			eosio_assert((is_roundenable(ROUND_TYPE0) == true), "round is full(disable)... Please select beneficiary");
			eosio_assert(is_account(receiver), "to account does not exist");
			print("refund donation to ", name{receiver}, "Sym = ", quantity.symbol.name(), "quantity.amount = ", quantity.amount);

			if(quantity.symbol == symbol_type(S(4, EOS)))
			{
				print(" Token = EOS\n");
				eosio_assert(MINBET == quantity.amount, "Must 1 EOS");
				require_auth2(N(goldenbucket), N(active));
				action(
					permission_level{_self, N(active)},
					N(eosio.token),
					N(transfer),
					std::make_tuple(
						_self,
						receiver,
						asset(quantity.amount, symbol_type(S(4, EOS))),
						std::string(" To:") + name_to_string(receiver) + std::string(" Amount= ") + std::to_string(quantity.amount) + std::string(" -- REFUND EOS.")
					)
				).send();
			}
			else if(quantity.symbol == symbol_type(S(4, KAKAO)))
			{
				print(" Token = KAKAO\n");
				eosio_assert(MINBET_KAKAO == quantity.amount, "Must 100 KAKAO");
				require_auth2(N(goldenbucket), N(active));
				action(
					permission_level{_self, N(active)},
					N(kakaofriends),
					N(transfer),
					std::make_tuple(
						_self,
						receiver,
						asset(quantity.amount, symbol_type(S(4, KAKAO))),
						std::string(" To:") + name_to_string(receiver) + std::string("Amount =") + std::to_string(quantity.amount) + std::string(" --REFUND KAKAO.")
					)
				).send();
			}
			else
			{
				eosio_assert(false, " Undefined Symbol!!!");
			}

		}

		// @abi action
		// cleos -u http://jungle.cryptolions.io:18888 push action goldenbucket closeround '["0", "0", "100.0000 EOS", "100.0000 KAKAO"]' -p goldenbucket
		void closeround(const uint8_t roundtype, const uint64_t round_index, const asset eosquantity, const asset goldquantity)
		{
			uint16_t range[5] = { 10, 500, 1000, 5000, 10000 };
			uint32_t winner_count = 0;
			uint32_t winner_number = 0; 

			eosio_assert(is_contract_freezed() == false, "contract is frozen");

			require_auth2(N(goldenbucket), N(active));
			//set_closeroundtransfer(true);

			for(int i = 0; i < range[roundtype] && winner_count == 0; i++)
			{
				winner_number = generate_luckynumber(range[roundtype]);
				eosio_assert((winner_number > 0 && winner_number <= range[roundtype]), " Invaild Beneficiary!!!");
				
				for(auto iter = round_info.begin(); iter != round_info.end(); iter++)
				{
					if(winner_number == iter->luckycode) 
					{ 
						winner_list.emplace(_self, [&](auto& st_winlist){
							st_winlist.round = iter->roundid; // need to modify....
							st_winlist.roundtype = roundtype;
							st_winlist.jointime = iter->jointime;
							st_winlist.beneficiery = iter->donator;
							st_winlist.luckycode = iter->luckycode;
							st_winlist.eos_amt = 0;
							st_winlist.gold_amt = 0;
							st_winlist.eos_payout = false;	
							st_winlist.gold_payout = false;	
							});
						winner_count++;
					}
				}
			}

			eosio_assert((winner_count > 0), " winner count is zero!!!");
			uint64_t prize_eos = (uint64_t)(90/winner_count);
			uint64_t prize_gold = (uint64_t)(500/winner_count);

			print("\nWinner Number = ", winner_number, " Winner Count=", winner_count, " Prize EOS=", prize_eos, " Prize Gold=", prize_gold);

			for(auto iter = winner_list.begin(); iter != winner_list.end(); iter++)
			{
				winner_list.modify(iter, 0, [&](auto& st_winlist){
					st_winlist.eos_amt = prize_eos;
					st_winlist.gold_amt = prize_gold;
					});

				std::string benf_str = name_to_string(iter->beneficiery);
				if(prize_eos > 0)
				{
					action(
						permission_level{_self, N(active)},
						N(eosio.token),
						N(transfer),
						std::make_tuple(
							_self,
							N(prizeswallet),
							asset(prize_eos*10000, symbol_type(S(4, EOS))),
							std::string("[Congratulation][EOS] : ") + benf_str + std::string("-- You are a Beneficiary! : donation.io")
						)
					).send();
					winner_list.modify(iter, 0, [&](auto& st_winlist){
						st_winlist.eos_payout = true;
						});
				}
				
				if(prize_gold > 0)
				{
					action(
						permission_level{_self, N(active)},
						N(kakaofriends),
						N(transfer),
						std::make_tuple(
							_self,
							N(prizeswallet),
							asset(prize_gold*10000, symbol_type(S(4, KAKAO))),
							std::string("[Congratulation][GOLD] : ") + name_to_string(iter->beneficiery) + std::string("-- You are a Beneficiary! : donation.io")
						)
					).send();
					winner_list.modify(iter, 0, [&](auto& st_winlist){
						st_winlist.gold_payout = true;
						});
				}

				// examples of deferred transactions being created, you must import eoslib/transaction.hpp
				transaction ref_tx{};

				ref_tx.actions.emplace_back(
					permission_level{_self, N(active)},
					_self,
					N(issueinvoice),
					std::make_tuple(
						std::string("[Congratulation!! - You are a Beneficiary]"),
						roundtype,
						iter->luckycode, 			// user code
						winner_number, 				// lucky number
						iter->round,				// roundid, sequential number - join order.
						iter->beneficiery,			// donator name
						asset(prize_eos, symbol_type(S(4, EOS))),		// EOS Payout
						asset(prize_gold, symbol_type(S(4, GOLD)))		// GOLD Payout
					)
				);

				ref_tx.delay_sec = 5;
				//the first parameter of transaction.send is the id of the transaction you want to create
				ref_tx.send(iter->round, _self);				
			}

			// set_closeroundtransfer(false);
			// delete table
			// round_control(0, true);

			// delete previouse ram table.......
			//round_info.erase(activeround_itr);
		}


		// @abi action
		void issueinvoice(std::string msg, uint8_t roundtype, uint32_t usercode, uint32_t luckynumber, uint64_t roundid, account_name donator, 
							asset eos_payout, asset gold_payout) 
		{
			eosio_assert(is_contract_freezed() == false, "contract is frozen");
			require_auth(_self);
 			require_recipient(donator);
		}


		uint32_t is_contribution_transfer(const std::string memo)
		{
			std::string header_str;

			//Contribution-1234-100
			const std::size_t first_break = memo.find("-");

			if(first_break != std::string::npos)
			{
				header_str = memo.substr(0, first_break);
				if(header_str.compare("Contribution") == 0) { return true; }
			}
			return false;
		}


		// cleos -u http://jungle.cryptolions.io:18888 push action eosio.token transfer '["iloveyoutube", "goldenbucket", "1.0000 EOS", "Contribution-223-1000"]' -p iloveyoutube
		uint32_t get_userluckycode(const std::string memo)
		{
			std::string header_str, luckynumber_str, postfix_str;
			uint32_t usercode = 0, max = 100;

			//Contribution-1234-100
			const std::size_t first_break = memo.find("-");
			header_str = memo.substr(0, first_break);

			if(first_break != std::string::npos)
			{
				const std::string after_first_break = memo.substr(first_break + 1); // 1234-Luckynumber
				const std::size_t second_break = after_first_break.find("-");

				if(second_break != std::string::npos)
				{
					luckynumber_str = after_first_break.substr(0, second_break);
					postfix_str = after_first_break.substr(second_break + 1);
					usercode = (uint32_t) std::stoi(luckynumber_str); // need to handle exception case 3A A45 ETC...
					max = (uint32_t) std::stoi(postfix_str);
					print("\nheader = ", header_str, " Lucky Number String = ", luckynumber_str, " Postfix = ", postfix_str, "Lucky Number = ", usercode, "\n");
				}
			}
			eosio_assert((usercode >= 1 && usercode <= max) , "Abnormal Lucky Number!!!");
			return usercode;
		}

		uint32_t get_joinround(const std::string memo)
		{
			std::string header_str, luckynumber_str, postfix_str;
			uint32_t max = 100;
			uint8_t roundtype = 0xff;

			//Contribution-1234-100
			const std::size_t first_break = memo.find("-");
			header_str = memo.substr(0, first_break);

			if(first_break != std::string::npos)
			{
				const std::string after_first_break = memo.substr(first_break + 1); // 1234-Luckynumber
				const std::size_t second_break = after_first_break.find("-");

				if(second_break != std::string::npos)
				{
					luckynumber_str = after_first_break.substr(0, second_break);
					postfix_str = after_first_break.substr(second_break + 1);
					max = (uint32_t) std::stoi(postfix_str);
					print("\nheader = ", header_str, " Lucky Number String = ", luckynumber_str, " Postfix = ", postfix_str, "\n");
				}
			}
			
			if(max == 100) { roundtype = ROUND_TYPE0;}
			else if(max == 500) { roundtype = ROUND_TYPE1; }
			else if(max == 1000) { roundtype = ROUND_TYPE2; }
			else if(max == 5000) { roundtype = ROUND_TYPE3; }
			else if(max == 10000) { roundtype = ROUND_TYPE4; }
			else { roundtype = 0xff; }
			eosio_assert((roundtype >= ROUND_TYPE0 && roundtype <= ROUND_TYPE4) , "Abnormal round type!!!");

			return roundtype;
		}


		// cleos -u http://jungle.cryptolions.io:18888 push action goldenbucket cfginit '[]' -p goldenbucket
		// cleos -u http://jungle.cryptolions.io:18888 get table goldenbucket goldenbucket stbucketcfgs
		// @abi action
		void cfginit()
		{
			require_auth(_self);

			auto bucketcfg = get_bucketcfg();
			bucketcfg.contract_enable = true;
			bucketcfg.is_closeround_transfer = false;
			bucketcfg.create_time	= time_point_sec(now()).utc_seconds;
		
			bucketcfg.round0_runnable = true; // 100
			bucketcfg.round1_runnable = true; // 500
			bucketcfg.round2_runnable = true; // 1000
			bucketcfg.round3_runnable = false;// 5000
			bucketcfg.round4_runnable = false;// 10000
			
			update_bucketcfg(bucketcfg);
			eosio::print("bucket config initialize : ", " cfg create time	", bucketcfg.create_time, " (sec)\n");
			eosio::print("bucket config initialize : ", " contract enable   ", bucketcfg.contract_enable	," \n");	
			eosio::print("bucket config initialize : ", " round_runnable[ROUND_TYPE0]  ", bucketcfg.round0_runnable	," \n");	
			eosio::print("bucket config initialize : ", " round_runnable[ROUND_TYPE1]  ", bucketcfg.round1_runnable	," \n");	
			eosio::print("bucket config initialize : ", " round_runnable[ROUND_TYPE2]  ", bucketcfg.round2_runnable	," \n");	
			eosio::print("bucket config initialize : ", " round_runnable[ROUND_TYPE3]  ", bucketcfg.round3_runnable	," \n");	
			eosio::print("bucket config initialize : ", " round_runnable[ROUND_TYPE4]  ", bucketcfg.round4_runnable	," \n");		
		}


		bool is_transferable(account_name to, account_name from)
		{
			if (is_blacklist(to) == false && is_blacklist(from) == false) { return true; }
			return false;
		}
		
		bool is_blacklist(account_name user)
		{
			auto bc = get_bucketcfg();
		
			vector<account_name>::iterator iter;
			iter = find(bc.blacklist.begin(), bc.blacklist.end(), user);
		
			return (iter == bc.blacklist.end()) ? false : true ;
		}
		
		// @abi action
		// cleos -u http://jungle.cryptolions.io:18888 push action goldenbucket addblacklist '["ilovelgtwins"]' -p goldenbucket
		void addblacklist(account_name user)
		{
			eosio_assert(is_contract_freezed() == false, "contract is frozen");
			eosio_assert(is_blacklist(user) == false, "account_name already exist in blacklist");
		
			require_auth(_self);
			auto bc = get_bucketcfg();
			bc.blacklist.push_back(user);
			update_bucketcfg(bc);
		}
		
		
		// @abi action
		// cleos -u http://jungle.cryptolions.io:18888 push action goldenbucket delblacklist '["ilovelgtwins"]' -p goldenbucket
		void delblacklist(account_name user)
		{
			eosio_assert(is_contract_freezed() == false, "contract is frozen");
			require_auth(_self);
		
			auto bc = get_bucketcfg();
			vector<account_name>::iterator iter;
			iter = find(bc.blacklist.begin(), bc.blacklist.end(), user);
			eosio_assert( iter != bc.blacklist.end(), "account_name not exist in blacklist");
			bc.blacklist.erase(iter);
		
			update_bucketcfg(bc);
		}
		

		bool is_contract_freezed()
		{
			auto bucketcfg = get_bucketcfg();

			return ((bucketcfg.contract_enable == false) ? true : false);
		}

		bool is_closeroundtransfer()
		{
			auto bucketcfg = get_bucketcfg();
			return ((bucketcfg.is_closeround_transfer == true) ? true : false);
		}

		void set_closeroundtransfer(bool enable)
		{
			auto bucketcfg = get_bucketcfg();
			
			bucketcfg.is_closeround_transfer = enable;
			update_bucketcfg(bucketcfg);
		}
		
		bool is_roundenable(uint8_t roundtype)
		{
			auto bucketcfg = get_bucketcfg();

			if(roundtype == ROUND_TYPE0) { return (bucketcfg.round0_runnable == true) ? true : false; }
			else if(roundtype == ROUND_TYPE1) { return (bucketcfg.round1_runnable == true) ? true : false; }
			else if(roundtype == ROUND_TYPE2) { return (bucketcfg.round2_runnable == true) ? true : false; }
			else if(roundtype == ROUND_TYPE3) { return (bucketcfg.round3_runnable == true) ? true : false; }
			else if(roundtype == ROUND_TYPE4) { return (bucketcfg.round4_runnable == true) ? true : false; }
			else { return false; }
		}

		void round_control(uint8_t roundtype, bool enable)
		{
			auto bucketcfg = get_bucketcfg();
			
			if(roundtype == ROUND_TYPE0) { bucketcfg.round0_runnable = enable; }
			else if(roundtype == ROUND_TYPE1) { bucketcfg.round1_runnable = enable; }
			else if(roundtype == ROUND_TYPE2) { bucketcfg.round2_runnable = enable; }
			else if(roundtype == ROUND_TYPE3) { bucketcfg.round3_runnable = enable; }
			else if(roundtype == ROUND_TYPE4) { bucketcfg.round4_runnable = enable; }
			else { }
			if(roundtype >= ROUND_TYPE0 && roundtype <= ROUND_TYPE4) { update_bucketcfg(bucketcfg); }
		}
		
		// @abi action
		// cleos -u http://jungle.cryptolions.io:18888 push action goldenbucket ctfreeze '[]' -p goldenbucket
		// cleos -u http://jungle.cryptolions.io:18888 get table goldenbucket goldenbucket stbucketcfgs
		void ctfreeze()
		{
			require_auth(_self);
			auto bucketcfg = get_bucketcfg();
			eosio_assert(bucketcfg.contract_enable == true, "contract is aleady frozen state! Skip freeze action.");
			eosio::print(" set contract to frozen state! \n ");
			bucketcfg.contract_enable = false;
			update_bucketcfg(bucketcfg);
		}
		
		
		// @abi action
		// cleos -u http://jungle.cryptolions.io:18888 push action goldenbucket ctunfreeze '[]' -p goldenbucket
		// cleos -u http://jungle.cryptolions.io:18888 get table goldenbucket goldenbucket stbucketcfgs
		void ctunfreeze()
		{
			require_auth(_self);
			auto bucketcfg = get_bucketcfg();
			eosio_assert(bucketcfg.contract_enable == false, "contract is already active state! Skip unfreeze action");
			eosio::print(" set contract to active state! \n ");
			bucketcfg.contract_enable = true;
			update_bucketcfg(bucketcfg);
		}
		
		
		stbucketcfg get_bucketcfg()
		{
			stbucketcfg bc;
		
			if(bucket_cfg.exists()) 
			{
				bc = bucket_cfg.get();
			}
			else
			{
				// need to check again... ??
				bc = stbucketcfg{};
				if (bucket_cfg.exists()) 
				{	
					auto old_bc = bucket_cfg.get();
					bc.contract_enable = old_bc.contract_enable;
 					bc.is_closeround_transfer = old_bc.is_closeround_transfer;
					bc.create_time = old_bc.create_time;
					bc.round0_runnable = old_bc.round0_runnable;
					bc.round1_runnable = old_bc.round1_runnable;
					bc.round2_runnable = old_bc.round2_runnable;
					bc.round3_runnable = old_bc.round3_runnable;
					bc.round4_runnable = old_bc.round4_runnable;
					
					bucket_cfg.remove();
					bucket_cfg.set(bc, _self);
				}
			}
		
			return bc;
		}
		
		
		void update_bucketcfg(stbucketcfg &new_bucket_cfg)
		{
			bucket_cfg.set(new_bucket_cfg, _self);
		}

		// https://eosio.stackexchange.com/questions/41/how-can-i-generate-random-numbers-inside-a-smart-contract
		// EOS Knights implements a random generator, which, at the moment, is MIT-licensed:
		// Linear Congruential Generator
	    uint32_t generate_luckynumber(uint32_t to)
	    {
			 // Calculates sha256(data,length) and stores result in memory pointed to by hash 
			 // `hash` should be checksum<256>
			 // void sha256( char* data, uint32_t length, checksum256* hash );
		
			uint64_t seed = current_time(); // + account_name;
	        checksum256 result;
	        sha256((char *)&seed, sizeof(seed), &result);
			for(int i = 0; i < 32; i++)
			{
				seed += result.hash[i];
			}
			/*
	        seed = result.hash[1];
	        seed <<= 32;
	        seed |= result.hash[0];
	        */
	        return (uint32_t)((seed % to)+1);

	        // old implementation
			// const uint32_t a = 1103515245;
			// const uint32_t c = 12345;
			// uint64_t seed = 0;
	        // seed = current_time() + player(account_name);
	        // seed = (a * seed + c) % 0x7fffffff;
	        // return (uint32_t)(seed % to);
    	}

	
		// @abi action
		// cleos -u http://jungle.cryptolions.io:18888 push action goldenbucket randkeytest '["10"]' -p goldenbucket
		void randkeytest(uint32_t to)
		{
			// Calculates sha256(data,length) and stores result in memory pointed to by hash 
			// `hash` should be checksum<256>
			// void sha256( char* data, uint32_t length, checksum256* hash );
		
			uint64_t seed = current_time();
			uint32_t ret_val = 0;
			checksum256 result;

			//require_auth(_self);
			
			sha256((char *)&seed, sizeof(seed), &result);
			for(int i = 0; i < 32; i++)
			{
				seed += result.hash[i];
			}
			
			ret_val = (uint32_t)((seed % to)+1);
			print("[Random Lucky Number] = ", ret_val, "\n");
			eosio_assert((false) , "[Random Lucky Number]");
		
			// old implementation
			// const uint32_t a = 1103515245;
			// const uint32_t c = 12345;
			// uint64_t seed = 0;
			// seed = current_time() + player(account_name);
			// seed = (a * seed + c) % 0x7fffffff;
			// return (uint32_t)(seed % to);
		}

};



// EOSIO_ABI_EX를 추가한 이유는 eosio.token의 transfer 함수에서 require_recipient( from );, require_recipient( to ); contract에서 notification 을 받아 처리할 수 있도록 함.
// require_recipient 기능을 이용하면 입금 기능말고 출금을 자동으로 로깅하게 할 수 있습니다. 컨트랙트에서 출금이 발생했을 때 지출 내역이라면 해당 내용을 기록 하는 것입니다.
// https://steemit.com/eos/@raindays/contract
// https://steemit.com/eos/@raindays/7uqisg-eos-knights-transfer-hack-statement
#if 0
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
#endif

#define EOSIO_ABI_EX( TYPE, MEMBERS ) \
extern "C" { \
   void apply( uint64_t receiver, uint64_t code, uint64_t action ) { \
      auto self = receiver; \
      if( code == self || code == N(eosio.token) || code == N(kakaofriends)) { \
      	 if( action == N(transfer)){ \
      	 	eosio_assert((code == N(eosio.token) || code == N(kakaofriends)), "Must transfer EOS or KAKAO"); \
      	 } \
         TYPE thiscontract( self ); \
         switch( action ) { \
            EOSIO_API( TYPE, MEMBERS ) \
         } \
         /* does not allow destructor of thiscontract to run: eosio_exit(0); */ \
      } \
   } \
}

EOSIO_ABI_EX(goldenbucket, (deltable)(droptable)(transfer)(refunddonate)(closeround)(issueinvoice)(cfginit)(addblacklist)(delblacklist)(ctfreeze)(ctunfreeze)(randkeytest))

// 1. EOS가 아닌 GOLD Token으로 전송시.... transfer 처리.... 어떻게 ?? token symbol / contract 비교 필요...

//[TEST Plan]
/*
1. contribution
cleos -u http://jungle.cryptolions.io:18888 push action eosio.token transfer '["iloveyoutube", "goldenbucket", "1.0000 EOS", "Contribution-1-100"]' -p iloveyoutube
cleos -u http://jungle.cryptolions.io:18888 push action kakaofriends transfer '["goldmonetary", "goldenbucket", "100.0000 KAKAO", "Contribution-8-100"]' -p goldmonetary

2. contribution list check
cleos -u http://jungle.cryptolions.io:18888 get table -l 100 goldenbucket goldenbucket roundinfos

3. close round
cleos -u http://jungle.cryptolions.io:18888 push action goldenbucket closeround '["0", "0", "100.0000 EOS", "100.0000 KAKAO"]' -p goldenbucket

4. winnerlist check
cleos -u http://jungle.cryptolions.io:18888 get table -l 100 goldenbucket goldenbucket winnerlists



10. etc...
1) delete item from table (deltable)
cleos -u http://jungle.cryptolions.io:18888 push action goldenbucket deltable '["round", "8398870303708059341"]' -p goldenbucket
cleos -u http://jungle.cryptolions.io:18888 push action goldenbucket deltable '["winnerlists", "1"]' -p goldenbucket

2) delete all data from table (droptable)
cleos -u http://jungle.cryptolions.io:18888 push action goldenbucket droptable '["round"]' -p goldenbucket
cleos -u http://jungle.cryptolions.io:18888 push action goldenbucket droptable '["winnerlists"]' -p goldenbucket

3) test lucky code generation
cleos -u http://jungle.cryptolions.io:18888 push action goldenbucket randkeytest '["10"]' -p goldenbucket
cleos -u http://jungle.cryptolions.io:18888 push action goldenbucket randkeytest '["100"]' -p goldenbucket
cleos -u http://jungle.cryptolions.io:18888 push action goldenbucket randkeytest '["500"]' -p goldenbucket
cleos -u http://jungle.cryptolions.io:18888 push action goldenbucket randkeytest '["1000"]' -p goldenbucket
cleos -u http://jungle.cryptolions.io:18888 push action goldenbucket randkeytest '["5000"]' -p goldenbucket
cleos -u http://jungle.cryptolions.io:18888 push action goldenbucket randkeytest '["10000"]' -p goldenbucket

4) cfg init / check
cleos -u http://jungle.cryptolions.io:18888 push action goldenbucket cfginit '[]' -p goldenbucket
cleos -u http://jungle.cryptolions.io:18888 get table goldenbucket goldenbucket stbucketcfgs

5) how to delete singleton(when structure was changed)
cleos -u http://jungle.cryptolions.io:18888 get table goldenbucket goldenbucket stbucketcfgs

item list to be solved
- transfer from/to assign.... - normal id - maybe OK... but account@eosio.code account - fail - Need to check again later.... (important)
- select winner algorithm - 
- display id name in memo correctly. - O
- how to delete singleton (when structure was changed) - O
- how to get transaction id / info....
  --> read_transaction(char * buffer,size_t size), but it seems not contain transaction id inside.
  -->  The raw transaction data does not contain the ID, but the ID is a calculated value. Your contract can recalculate it
      (https://github.com/EOSIO/eos/issues/4980)


  

cleos -u http://jungle.cryptolions.io:18888 push action goldenbucket cfginit '[]' -p goldenbucket
cleos -u http://jungle.cryptolions.io:18888 get table goldenbucket goldenbucket stbucketcfgs

cleos -u http://jungle.cryptolions.io:18888 push action goldenbucket droptable '["winnerlists"]' -p goldenbucket
cleos -u http://jungle.cryptolions.io:18888 get table -l 100 goldenbucket goldenbucket winnerlists

cleos -u http://jungle.cryptolions.io:18888 get table -l 100 goldenbucket goldenbucket roundinfos
cleos -u http://jungle.cryptolions.io:18888 push action goldenbucket deltable '["round", "11191290744178809300"]' -p goldenbucket

cleos -u http://jungle.cryptolions.io:18888 push action kakaofriends transfer '["iloveyoutube", "goldenbucket", "100.0000 KAKAO", "Contribution-3-100"]' -p iloveyoutube

cleos -u http://jungle.cryptolions.io:18888 push action goldenbucket closeround '["0", "0", "100.0000 EOS", "100.0000 KAKAO"]' -p goldenbucket
cleos -u http://jungle.cryptolions.io:18888 get table -l 100 goldenbucket goldenbucket winnerlists

*/
