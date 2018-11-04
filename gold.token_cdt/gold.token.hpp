/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/time.hpp>
#include <eosiolib/singleton.hpp>

#include <string>
#include <map>

using namespace std;
using namespace eosio;

namespace eosiosystem {
   class system_contract;
}

namespace eosgold {

	class goldtoken : public contract {

		public:
			struct transfer_args {
				account_name  from;
				account_name  to;
				asset         quantity;
				string        memo;
			};
			
			goldtoken( account_name self ):contract(self),
										    token_cfg(_self, _self)
			{ }

			void create( account_name issuer, asset maximum_supply );

			void issue( account_name to, asset quantity, string memo );

			void transfer( account_name from,
						   account_name to,
						   asset        quantity,
						   string       memo );

			inline asset get_supply( symbol_name sym )const;

			inline asset get_balance( account_name owner, symbol_name sym )const;

			// newly added
			void goldtoken_cfginit(void);

			// newly added
			void supplement( asset quantity );

			// newly added
			void burn( asset quantity );

			// newly added
			void addblacklist(account_name user, asset symbol);

			// newly added
			void delblacklist(account_name user, asset symbol);

			// newly added
			inline void ctfreeze(asset symbol);

			// newly added
			inline void ctunfreeze(asset symbol);

			// newly added
			void deltoken(string symbol);

			// newly added
			void delaccount( string symbol, account_name account);
			


		private:
			// @abi table accounts i64
			struct account {
				asset    balance;

				uint64_t primary_key()const { return balance.symbol.name(); }
			};

			// @abi table stat i64
			struct cur_stats {
				asset          supply;
				asset          max_supply;
				account_name   issuer;

				uint64_t primary_key()const { return supply.symbol.name(); }
			};

			typedef eosio::multi_index<N(accounts), account> accounts;
			typedef eosio::multi_index<N(stat), cur_stats> stats;

			void sub_balance( account_name owner, asset value );
			void add_balance( account_name owner, asset value, account_name ram_payer );

			// newly added
			// @abi table tokencfgs i64
			struct tokencfg {
				bool				 is_runnable;
				uint64_t			 create_time;
				uint32_t			 max_period;
				uint32_t			 lock_period;
				uint32_t			 period_uint;
				uint64_t			 lock_balance;
				uint64_t			 balance_uint;
				vector<account_name> blacklist;
			};

			// sigletone has only one instance and serve global access.
			typedef singleton<N(tokencfgs), tokencfg> tokencfg_singleton;

			tokencfg_singleton		token_cfg;

			tokencfg get_tokencfg(void);

			void update_tokencfg(tokencfg &new_token_cfg);

			bool is_runnable(void);

			bool is_valid_account(account_name to);

			inline bool is_transferable(account_name from, account_name to);

			bool is_blacklist(account_name user);

			uint64_t available_balance (uint64_t contract_balance);

			void auth_check(asset symbol);
	};


	asset goldtoken::get_supply( symbol_name sym )const
	{
		stats statstable( _self, sym );
		const auto& st = statstable.get( sym );
		return st.supply;
	}


	asset goldtoken::get_balance( account_name owner, symbol_name sym )const
	{
		accounts accountstable( _self, owner );
		const auto& ac = accountstable.get( sym );
		return ac.balance;
	}
} /// namespace eosio
