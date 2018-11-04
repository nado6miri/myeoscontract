/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include "gold.token.hpp"


// Clearing RAM tables of eosio.token contract
// https://eosio.stackexchange.com/questions/1462/clearing-ram-tables-of-eosio-token-contract?noredirect=1&lq=1
// cleos get table <CONTRACT_ACCOUNT> <TOKEN_NAME> stat
// cleos get table <CONTRACT_ACCOUNT> <AIRDROPPED_ACCOUNT> accounts


namespace eosgold {

void goldtoken::create(account_name issuer, asset maximum_supply)
{
    require_auth(_self);

    auto sym = maximum_supply.symbol;
    eosio_assert(sym.is_valid(), "invalid symbol name");
    eosio_assert(maximum_supply.is_valid(), "invalid supply");
    eosio_assert(maximum_supply.amount > 0, "max-supply must be positive");

    stats statstable(_self, sym.name());
    auto existing = statstable.find(sym.name());
    eosio_assert(existing == statstable.end(), "goldtoken with symbol already exists");

	goldtoken_cfginit();

    statstable.emplace( _self, [&]( auto& s ) {
		    s.supply.symbol = maximum_supply.symbol;
		    s.max_supply    = maximum_supply;
		    s.issuer        = issuer;
		    });
}


void goldtoken::issue(account_name to, asset quantity, string memo)
{
	auto sym = quantity.symbol;
	eosio_assert( is_account( to ), "to account does not exist");
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    auto sym_name = sym.name();
    stats statstable( _self, sym_name );
    auto existing = statstable.find( sym_name );
    eosio_assert( existing != statstable.end(), "goldtoken with symbol does not exist, create goldtoken before issue" );
    const auto& st = *existing;

	require_auth( st.issuer );

	eosio_assert( is_runnable() == true, "contract was frozen");

	eosio_assert( is_blacklist(to) == false, "not allowed issue to black list");


	eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must issue positive quantity" );

    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    eosio_assert( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

	/*
	// Newly Add : check locking amount
	eosio_assert(quantity.amount <= available_balance(st.max_supply.amount - st.supply.amount),
					"currency has locked... please check lock time and amount");
	*/
	
	statstable.modify(st, 0, [&](auto& s) { s.supply += quantity; });
    add_balance( st.issuer, quantity, st.issuer );

    if( to != st.issuer ) 
	{
       SEND_INLINE_ACTION(*this, transfer, {st.issuer,N(active)}, {st.issuer, to, quantity, memo});
	}
}


void goldtoken::transfer(account_name from, account_name to, asset quantity, string memo)
{
	eosio_assert(from != to, "cannot transfer to self");

	// Newly Add : check black list.
	eosio_assert(is_runnable() == true, "contract was frozen");
	eosio_assert(is_transferable(from, to) == true ,"not allow transfer!");

	require_auth(from);
	eosio_assert(is_account(to), "to account does not exist");

    auto sym = quantity.symbol.name();
    stats statstable( _self, sym );
    const auto& st = statstable.get( sym );

	require_recipient( from );
    require_recipient( to );

    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    sub_balance( from, quantity );
	add_balance( to, quantity, from );
}


void goldtoken::sub_balance(account_name owner, asset value) {
   	accounts from_acnts(_self, owner);

   	const auto& from = from_acnts.get(value.symbol.name(), "no balance object found");
   	eosio_assert(from.balance.amount >= value.amount, "overdrawn balance");

   	if(from.balance.amount == value.amount) 
   	{
    	from_acnts.erase(from);
   	} 
	else 
	{
    	from_acnts.modify(from, owner, [&](auto& a){ a.balance -= value; });
   	}
}


void goldtoken::add_balance(account_name owner, asset value, account_name ram_payer)
{
	accounts to_acnts( _self, owner );
   	auto to = to_acnts.find( value.symbol.name() );
   	if(to == to_acnts.end()) 
   	{
    	to_acnts.emplace(ram_payer, [&](auto& a){ a.balance = value; });
   	} 
	else 
	{
    	to_acnts.modify(to, 0, [&](auto& a){ a.balance += value; });
   	}
}

/*
 ========================================================================================
*/
void goldtoken::goldtoken_cfginit()
{
	auto tokencfg = get_tokencfg();
	tokencfg.is_runnable	= true;
	tokencfg.create_time	= time_point_sec(now()).utc_seconds;

	tokencfg.max_period		= 12;
    tokencfg.period_uint	= 30 * 86400; // 30 day
    tokencfg.lock_period	= 3; // 90 days holding after unlock 1 bilions per 30 days
    tokencfg.balance_uint	= 1000000000000;
    tokencfg.lock_balance	= 9000000000000; // 9 bilions - it unlock when token created after 360 days

	update_tokencfg(tokencfg);
	eosio::print("token config initialize : ", " token create time  ", tokencfg.create_time, " (sec)\n");
	eosio::print("token config initialize : ", " max_period	  ", tokencfg.max_period	," \n");	
	eosio::print("token config initialize : ", " lock_period  ", tokencfg.lock_period	," \n");	
	eosio::print("token config initialize : ", " period_uint  ", tokencfg.period_uint, " (sec)\n");	
	eosio::print("token config initialize : ", " amount_uint  ", tokencfg.balance_uint, " \n");	
	eosio::print("token config initialize : ", " lock_balance ", tokencfg.lock_balance, " \n");	

}


void goldtoken::supplement(asset quantity)
{
	auto sym = quantity.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );

	// Newly Add
	eosio_assert( is_runnable() == true, "contract was frozen");

	auto sym_name = sym.name();
	stats statstable( _self, sym_name );
	auto existing = statstable.find( sym_name );
	eosio_assert( existing != statstable.end(), "token with symbol does not exist, create token before supplement" );

	const auto& st = *existing;

    require_auth( st.issuer );
    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must issue positive quantity" );
    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );

    statstable.modify(st, 0, [&](auto& s) { s.max_supply += quantity; });
}


void goldtoken::burn(asset quantity)
{
	auto sym = quantity.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
	eosio_assert( is_runnable() == true, "contract was frozen");

    auto sym_name = sym.name();
	stats statstable( _self, sym_name );
	auto existing = statstable.find( sym_name );
	eosio_assert( existing != statstable.end(), "goldtoken with symbol does not exist, create goldtoken before burn" );

	const auto& st = *existing;

    require_auth( st.issuer);
    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must issue positive quantity" );
    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );

	eosio_assert( (st.max_supply.amount - st.supply.amount) >= quantity.amount , "Burn has allow when value of burn have big amount more than remain value");

    statstable.modify(st, 0, [&](auto& s){ s.max_supply -= quantity; });
}


bool goldtoken::is_transferable(account_name to, account_name from)
{
	if (is_blacklist(to) == false && is_blacklist(from) == false) { return true; }
	return false;
}


void goldtoken::addblacklist(account_name user ,asset symbol)
{
	eosio_assert(is_blacklist(user) == false, "account_name already exist in blacklist");

	auth_check(symbol);
	auto tc = get_tokencfg();
	tc.blacklist.push_back(user);
	update_tokencfg(tc);
}


void goldtoken::delblacklist(account_name user ,asset symbol)
{
	auth_check(symbol);

	auto tc = get_tokencfg();
	vector<account_name>::iterator iter;
	iter = find(tc.blacklist.begin(), tc.blacklist.end(), user);
	eosio_assert( iter != tc.blacklist.end(), "account_name not exist in blacklist");
	tc.blacklist.erase(iter);

	update_tokencfg(tc);
}


bool goldtoken::is_runnable()
{
	auto tokencfg = get_tokencfg();
	return (tokencfg.is_runnable ==  true) ? true : false;
}


void goldtoken::ctfreeze(asset symbol)
{
	auth_check(symbol);

	auto tokencfg = get_tokencfg();

	eosio_assert(tokencfg.is_runnable == true, "contract is aleady frozen state! Skip freeze action.");
	eosio::print(" set contract to frozen state! \n ");
	tokencfg.is_runnable = false;

	update_tokencfg(tokencfg);
}


void goldtoken::ctunfreeze(asset symbol)
{
	auth_check(symbol);

	auto tokencfg = get_tokencfg();

	eosio_assert(tokencfg.is_runnable == false, "contract is already active state! Skip unfreeze action");
	eosio::print(" set contract to active state! \n ");
	tokencfg.is_runnable = true;

	update_tokencfg(tokencfg);
}


bool goldtoken::is_blacklist(account_name user)
{
	auto tc = get_tokencfg();

	vector<account_name>::iterator iter;
	iter = find(tc.blacklist.begin(), tc.blacklist.end(), user);

	return (iter == tc.blacklist.end()) ? false : true ;
}


void goldtoken::auth_check(asset symbol)
{
	auto sym = symbol.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );

    auto sym_name = sym.name();
	// _self means running contract account
	// 1st _self : owner of table
	// 2st _self : user who use this table
	stats statstable( _self, sym_name );

    auto existing = statstable.find( sym_name );
    eosio_assert( existing != statstable.end(), "does not exist token with symbol" );

	const auto& st = *existing;
	require_auth( st.issuer );
}


goldtoken::tokencfg goldtoken::get_tokencfg()
{
	tokencfg tc;

	if(token_cfg.exists()) 
	{
		tc = token_cfg.get();
	}
	else
	{
		// need to check again... ??
		tc = tokencfg{};
		if (token_cfg.exists()) 
		{
			auto old_tc = token_cfg.get();
			tc.is_runnable = old_tc.is_runnable;
            tc.create_time = old_tc.create_time;
			token_cfg.remove();
		}
		token_cfg.set(tc, _self);
	}

	return tc;
}


void goldtoken::update_tokencfg(goldtoken::tokencfg &new_token_cfg)
{
	token_cfg.set(new_token_cfg, _self);
}


void goldtoken::deltoken(string symbol)
{
	require_auth( _self );

	symbol_type sym = string_to_symbol(0, symbol.c_str());

	stats statstable( _self, sym.name() );
	auto existing = statstable.find( sym.name() );
	eosio_assert( existing != statstable.end(), "token with symbol does not exist" );

	statstable.erase( existing );
}

void goldtoken::delaccount(string symbol, account_name account)
{
	require_auth( _self );

	symbol_type sym = string_to_symbol(0, symbol.c_str());

	accounts acctable( _self, account );
	const auto& row = acctable.get( sym.name(), "no balance object found for provided account and symbol" );
	acctable.erase( row );
}


/*
 *                                Token holding balance per period.
 * 
 *===============================================================================================================
 * day after    |  30d |  60d |  90d |  120d |  150d |  180d |  210d |  240d | 270d  |  300d |  330d | 360d | 
 * --------------------------------------------------------------------------------------------------------------
 * period       |  1p  |  2p  |  3p  |   4p  |   5p  |   6p  |   7p  |   8p  |   9p  |  10p  |  11p  |  12p | 
 * --------------------------------------------------------------------------------------------------------------
 * hold balance | 1.8b | 1.8b | 1.8b |  1.7b |  1.6b |  1.5b |  1.4b |  1.3b |  1.2b |  1.1b |  1.0b | 0.9b | 0b
 *===============================================================================================================
 *
 *
 */
/*
uint64_t goldtoken::available_balance( uint64_t contract_balance )
{
	uint64_t available_balance;

	auto tc = get_tokencfg();

	if ( tc.max_period > 0)
	{
		uint32_t current_time = time_point_sec(now()).utc_seconds;

		if ( ((current_time - tc.create_time) / (tc.lock_period * tc.period_uint)) > 0)
		{
			uint32_t current_period = (current_time - tc.create_time) / tc.period_uint + 1;

			if ( current_period <= tc.max_period)
			{
				uint64_t current_lock_balance =  tc.balance_uint * (tc.max_period - current_period);

				available_balance = contract_balance - current_lock_balance - tc.lock_balance ;

				eosio::print(" period   : ", current_period, " / ",  tc.max_period, " available balance : ", available_balance, " ");
			}
			else
			{
				available_balance = contract_balance;

				if (tc.max_period > 0)
				{
					tc.max_period = 0;
					update_tokencfg(tc);
				}
			}
		}
		else
		{
			// hold 18 billions within 90 days.
			available_balance = contract_balance - ( (tc.max_period - tc.lock_period) * tc.balance_uint) - tc.lock_balance;
		}
	}

	return available_balance;

}
*/

} // namespace eosgold

EOSIO_ABI( eosgold::goldtoken, (create)(issue)(transfer)(addblacklist)(delblacklist)(ctfreeze)(ctunfreeze)(supplement)(burn)(deltoken)(delaccount))


//==================================================================================
// Create Token and issue / transfer test
//==================================================================================
// 1. set contract gold.token to account mercedezbenz

// 2. create token.. 
/*
# cleos -u http://jungle.cryptolions.io:18888 push action mercedezbenz create '["mercedezbenz", "1000000000.0000 KAKAO"]' -p mercedezbenz
executed transaction: 772bbd6d39ba693d57fa46d2507cc99d9d0f702d153de5a33c790d9202ed39ac  120 bytes  460 us
#  mercedezbenz <= mercedezbenz::create         {"issuer":"mercedezbenz","maximum_supply":"1000000000.0000 KAKAO"}
>> token config initialize :  token create time  1538213035 (sec)
*/


// 3. issue
// cleos -u http://jungle.cryptolions.io:18888 push action mercedezbenz issue '["goldbucket00", "10000.0000 KAKAO", "memo: KAKAO - thanks airdrop"]' -p mercedezbenz
/*
executed transaction: b45244530ef92cc0f484807db3233d1fe379e754440ae7071247ffe85d7df6ff  152 bytes  1440 us
#  mercedezbenz <= mercedezbenz::issue          {"to":"goldbucket00","quantity":"10000.0000 KAKAO","memo":"memo: KAKAO - thanks airdrop"}
#  mercedezbenz <= mercedezbenz::transfer       {"from":"mercedezbenz","to":"goldbucket00","quantity":"10000.0000 KAKAO","memo":"memo: KAKAO - thank...
#  goldbucket00 <= mercedezbenz::transfer       {"from":"mercedezbenz","to":"goldbucket00","quantity":"10000.0000 KAKAO","memo":"memo: KAKAO - thank...
*/

// 4. transfer test
// cleos -u http://jungle.cryptolions.io:18888 push action mercedezbenz transfer '["goldbucket00", "mercedezbenz", "1000.0000 KAKAO", "token transfer test"]' -p goldbucket00
/*
executed transaction: 7786d8d0f05f99a97d8badf0bb8533afeeb0b8029f10b1e4e22e8ce97511af31  144 bytes  704 us
#  mercedezbenz <= mercedezbenz::transfer       {"from":"goldbucket00","to":"mercedezbenz","quantity":"1000.0000 KAKAO","memo":"token transfer test"...
#  goldbucket00 <= mercedezbenz::transfer       {"from":"goldbucket00","to":"mercedezbenz","quantity":"1000.0000 KAKAO","memo":"token transfer test"...
*/

//cleos -u http://jungle.cryptolions.io:18888 push action mercedezbenz transfer '["mercedezbenz", "goldbucket00", "500.0000 KAKAO", "token transfer test"]' -p mercedezbenz
/*
executed transaction: ec8cba3a080ce2f53a1b2cbf773f5a53adf07584a0045b2cd279540fc65fcb67  144 bytes  560 us
#  mercedezbenz <= mercedezbenz::transfer       {"from":"mercedezbenz","to":"goldbucket00","quantity":"500.0000 KAKAO","memo":"token transfer test"}
#  goldbucket00 <= mercedezbenz::transfer       {"from":"mercedezbenz","to":"goldbucket00","quantity":"500.0000 KAKAO","memo":"token transfer test"}
*/

// 5. stat table check
// cleos get table <CONTRACT_ACCOUNT> <TOKEN_NAME> stat
// cleos -u http://jungle.cryptolions.io:18888 get table mercedezbenz KAKAO stat
/*
{
  "rows": [{
      "supply": "10000.0000 KAKAO",
      "max_supply": "1000000000.0000 KAKAO",
      "issuer": "mercedezbenz"
    }
  ],
  "more": false
}
*/

// 6. accounts table check
// cleos get table <CONTRACT_ACCOUNT> <AIRDROPPED_ACCOUNT> accounts
// cleos -u http://jungle.cryptolions.io:18888 get table mercedezbenz goldbucket00 accounts
/*
{
  "rows": [{
      "balance": "9500.0000 KAKAO"
    }
  ],
  "more": false
}
*/

// 7. get token cfg table
// cleos -u http://jungle.cryptolions.io:18888 get table mercedezbenz mercedezbenz tokencfgs
/*
{
  "rows": [{
      "is_runnable": 1,
      "create_time": 1538213035,
      "max_period": 12,
      "lock_period": 3,
      "period_uint": 2592000,
      "lock_balance": "9000000000000",
      "balance_uint": "1000000000000",
      "blacklist": []
    }
  ],
  "more": false
}
*/

// 8. add / delete black list
// add : cleos -u http://jungle.cryptolions.io:18888 push action mercedezbenz addblacklist '["goldbucket02", "0.0000 KAKAO"]' -p mercedezbenz
/*
executed transaction: 37af3f897a35336f0e0b6f7c1325f7081ad0f26bba437ade4fed5a2cb2e18d9d  120 bytes  855 us
#  mercedezbenz <= mercedezbenz::addblacklist   {"user":"goldbucket02","symbol":"0.0000 KAKAO"}

// cleos -u http://jungle.cryptolions.io:18888 get table mercedezbenz mercedezbenz tokencfgs
{
  "rows": [{
      "is_runnable": 1,
      "create_time": 1538213035,
      "max_period": 12,
      "lock_period": 3,
      "period_uint": 2592000,
      "lock_balance": "9000000000000",
      "balance_uint": "1000000000000",
      "blacklist": [
        "goldbucket02",
        "goldbucket01"
      ]
    }
  ],
  "more": false
}
// Black list token transfer....
// cleos -u http://jungle.cryptolions.io:18888 push action mercedezbenz transfer '["mercedezbenz", "goldbucket01", "1.0000 KAKAO", "token transfer test"]' -p mercedezbenz
Error 3050003: eosio_assert_message assertion failure
Error Details:
assertion failure with message: not allow transfer!
*/
// del : cleos -u http://jungle.cryptolions.io:18888 push action mercedezbenz delblacklist '["goldbucket02", "0.0000 KAKAO"]' -p mercedezbenz
/*
executed transaction: bcd22c0499a4e1b0530ffcde73990b100dc12b319cec50874c13a7f85187cfaa  120 bytes  470 us
#  mercedezbenz <= mercedezbenz::delblacklist   {"user":"goldbucket02","symbol":"0.0000 KAKAO"}

// cleos -u http://jungle.cryptolions.io:18888 get table mercedezbenz mercedezbenz tokencfgs
{
  "rows": [{
      "is_runnable": 1,
      "create_time": 1538213035,
      "max_period": 12,
      "lock_period": 3,
      "period_uint": 2592000,
      "lock_balance": "9000000000000",
      "balance_uint": "1000000000000",
      "blacklist": [
        "goldbucket01"
      ]
    }
  ],
  "more": false
}

// cleos -u http://jungle.cryptolions.io:18888 push action mercedezbenz transfer '["mercedezbenz", "goldbucket02", "1.0000 KAKAO", "token transfer test"]' -p mercedezbenz
executed transaction: 803f7aa06d94ba5d6fa196a455d9c80458efb3a6d4dc8c594d7846e51309314c  144 bytes  894 us
#  mercedezbenz <= mercedezbenz::transfer       {"from":"mercedezbenz","to":"goldbucket02","quantity":"1.0000 KAKAO","memo":"token transfer test"}
#  goldbucket02 <= mercedezbenz::transfer       {"from":"mercedezbenz","to":"goldbucket02","quantity":"1.0000 KAKAO","memo":"token transfer test"}

// cleos -u http://jungle.cryptolions.io:18888 get table mercedezbenz goldbucket02 accounts
{
  "rows": [{
      "balance": "1.0000 KAKAO"
    }
  ],
  "more": false
}
*/

// 9. freeze contract
// cleos -u http://jungle.cryptolions.io:18888 push action mercedezbenz ctfreeze '["0.0000 KAKAO"]' -p mercedezbenz
/*
executed transaction: fa591a590ad11a4a0ed02cf4670c8dc49fd0e0dbb94dae4ec57c8abadd9354d0  112 bytes  784 us
#  mercedezbenz <= mercedezbenz::ctfreeze       {"symbol":"0.0000 KAKAO"}
>>  set contract to frozen state! 

// cleos -u http://jungle.cryptolions.io:18888 get table mercedezbenz mercedezbenz tokencfgs
{
  "rows": [{
      "is_runnable": 0,
      "create_time": 1538213035,
      "max_period": 12,
      "lock_period": 3,
      "period_uint": 2592000,
      "lock_balance": "9000000000000",
      "balance_uint": "1000000000000",
      "blacklist": [
        "goldbucket01"
      ]
    }
  ],
  "more": false
}
*/

// 10. unfreeze contract
// cleos -u http://jungle.cryptolions.io:18888 push action mercedezbenz ctunfreeze '["0.0000 KAKAO"]' -p mercedezbenz
/*
executed transaction: 9f1376fe4be5705a9abe79eb7ca4f9cdc179c82601f0ef2a0376c80f04770511  112 bytes  779 us
#  mercedezbenz <= mercedezbenz::ctunfreeze     {"symbol":"0.0000 KAKAO"}
>>  set contract to active state! 

cleos -u http://jungle.cryptolions.io:18888 get table mercedezbenz mercedezbenz tokencfgs
{
  "rows": [{
      "is_runnable": 1,
      "create_time": 1538213035,
      "max_period": 12,
      "lock_period": 3,
      "period_uint": 2592000,
      "lock_balance": "9000000000000",
      "balance_uint": "1000000000000",
      "blacklist": [
        "goldbucket01"
      ]
    }
  ],
  "more": false
}
*/

// 11. delete account
// mercedezbenz : 342.38kb --> 342.27kb
// goldbucket00 : 522.23kb --> 522.10kb
// cleos -u http://jungle.cryptolions.io:18888 get table mercedezbenz goldbucket00 accounts
/*
{
"rows": [{
   "balance": "9500.0000 KAKAO"
 }
],
"more": false
}

// cleos -u http://jungle.cryptolions.io:18888 push action mercedezbenz delaccount '["KAKAO", "goldbucket00"]' -p mercedezbenz
executed transaction: 2bf0f7d6b9b30598f266818fb01bd25c86c7fe17036ba30643cc9be54134f362  112 bytes  644 us
#  mercedezbenz <= mercedezbenz::delaccount     {"symbol":"KAKAO","account":"goldbucket00"}

// cleos -u http://jungle.cryptolions.io:18888 get table mercedezbenz goldbucket00 accounts
{
  "rows": [],
  "more": false
}

*/
 
// 12. delete token - delete stat table
// cleos -u http://jungle.cryptolions.io:18888 push action mercedezbenz deltoken '["KAKAO"]' -p mercedezbenz
// mercedezbenz(499 KAKAO / st 1 billion) : 342.27kb --> 342.01kb
// goldbucket00(delete account - 0) : 522.10kb --> 522.10kb
// goldbucket02(1 KAKAO) : 4.10kb --> 4.10kb
/*
executed transaction: 49547c8379b403ca684bc8cfd26cab16a18bfb385554580e2eea5b302f11f969  104 bytes  805 us
#  mercedezbenz <= mercedezbenz::deltoken       {"symbol":"KAKAO"}

// cleos -u http://jungle.cryptolions.io:18888 get table mercedezbenz KAKAO stat
{
  "rows": [],
  "more": false
}

//cleos -u http://jungle.cryptolions.io:18888 get table mercedezbenz mercedezbenz accounts
{
  "rows": [{
      "balance": "499.0000 KAKAO"
    }
  ],
  "more": false
}
*/

// 13. token supplement
// cleos -u http://jungle.cryptolions.io:18888 push action mercedezbenz supplement '["1000000000.0000 KAKAO"]' -p mercedezbenz
/*
executed transaction: 378d3e3ac755c7436f743a73b95004c9b317cba586978f73651133e7f797481a  112 bytes  820 us
#  mercedezbenz <= mercedezbenz::supplement     {"quantity":"1000000000.0000 KAKAO"}

// cleos -u http://jungle.cryptolions.io:18888 get table mercedezbenz KAKAO stat
{
  "rows": [{
      "supply": "0.0000 KAKAO",
      "max_supply": "2000000000.0000 KAKAO",
      "issuer": "mercedezbenz"
    }
  ],
  "more": false
}

*/

// 14. burn token
// cleos -u http://jungle.cryptolions.io:18888 push action mercedezbenz burn '["1000000000.0000 KAKAO"]' -p mercedezbenz
/*
executed transaction: 32f96cffc299a5a2401acafc987ae589a73f19b3ab3e43ea9cad27bdd985ca36  112 bytes  660 us
#  mercedezbenz <= mercedezbenz::burn           {"quantity":"1000000000.0000 KAKAO"}

// cleos -u http://jungle.cryptolions.io:18888 get table mercedezbenz KAKAO stat
{
  "rows": [{
      "supply": "0.0000 KAKAO",
      "max_supply": "1000000000.0000 KAKAO",
      "issuer": "mercedezbenz"
    }
  ],
  "more": false
}
*/
