#include "api.hpp"
#include <string>
#include <set>
#include <string>
#include <map>
#include <utility>
#include <iostream>
#include <algorithm>
using namespace std;

// Конвертация ДКА в регулярное выражение методом исключения состояний

string concat_rv_or(string rv1, string rv2)
{
	return rv1 + "|" + rv2;
}

string concat_rv_and(string rv1, string rv2)
{
	return rv1 + rv2;
}


std::string dfa2re(DFA &d)
{
	if(d.is_empty())
		return "";

	auto alph = d.get_alphabet();
	auto states = d.get_states();
	auto initial = d.get_initial_state();

	// Удаление недостежимых состояний из ДКА
	set<string> reachable;
	reachable.insert(initial);
	bool added_state = true;
	while(added_state == true)
	{
		added_state = false;
		for(auto i = reachable.begin(); i != reachable.end(); i++)
		{
			for(auto ch = alph.begin(); ch != alph.end(); ch++)
			{
				if(d.has_trans(*i, *ch))
				{
					bool updated = reachable.insert(d.get_trans(*i, *ch)).second;
					added_state = added_state || updated;
				}
			}
		}
	}
	for(auto i = states.begin(); i != states.end(); i++)
	{
		if(reachable.find(*i) == reachable.end())
		{
			d.delete_state(*i);
		}
	}
	if (d.is_empty())
		return "";

	//Добавление фиктивных состояний начала и конца для облегчения построения РВ
	alph.insert('#');
	DFA extended(alph);
	extended.create_state("new_start");
	extended.set_initial("new_start");
	extended.create_state("new_end", true);
	states = d.get_states();
	for(auto i = states.begin(); i != states.end(); i++)
	{
		extended.create_state(*i);
		if(d.is_final(*i))
		{
			extended.set_trans(*i, '#', "new_end");
		}
	}
	extended.set_trans("new_start", '#', initial);
	for(auto i = states.begin(); i != states.end(); i++)
	{
		for(auto ch = alph.begin(); ch != alph.end(); ch++)
		{
			if(d.has_trans(*i, *ch))
			{
				extended.set_trans(*i, *ch, d.get_trans(*i, *ch));
			}
		}
	}
	states = extended.get_states();
	map<pair<string, string>, string> markings; // Метки решулярных выражений, соответсвтующие переходам ДКА
	map<string, set<string> > from_here;
	map<string, set<string> > to_here;

	// Инициализация меток
	for(auto i = states.begin(); i != states.end(); i++)
	{
		set<string> tmp;
		from_here[*i] = tmp;
		to_here[*i] = tmp;
	}
	for(auto i = states.begin(); i != states.end(); i++)
	{
		for(auto ch = alph.begin(); ch != alph.end(); ch++)
		{
			if(extended.has_trans(*i, *ch))
			{
				auto st = extended.get_trans(*i, *ch);
				if(st != *i)
				{
					from_here[*i].insert(st);
					to_here[st].insert(*i);
				}
				pair<string, string> tmp(*i, st);
				if(markings.find(tmp) == markings.end())
				{
					markings[tmp] = string(1, *ch);
				}
				else
				{
					markings[tmp] = markings[tmp].append("|");
					markings[tmp] = markings[tmp].append(string(1, *ch));
				}
			}
		}
	}
	
	// Последовательное удаление состояний с соответсвующей модификацией меток
	for(auto i = states.begin(); i != states.end(); i++)
	{
		if(!extended.is_final(*i) && !extended.is_initial(*i))
		{
			pair<string, string> loop(*i, *i);
			for(auto from = to_here[*i].begin(); from != to_here[*i].end(); from++)
			{
				for(auto to = from_here[*i].begin(); to != from_here[*i].end(); to++)
				{
					if (*from != *i && *to != *i)
					{
						from_here[*from].insert(*to);
						to_here[*to].insert(*from);
						from_here[*from].erase(*i);
						to_here[*to].erase(*i);
						pair<string, string> tmp(*from, *to);
						pair<string, string> pair_to(*i, *to);
						pair<string, string> pair_from(*from, *i);
						string S = "";
						if (markings.find(loop) != markings.end())
						{
							S = markings[loop];
						}
						if (markings.find(tmp) != markings.end())
						{
							markings[tmp] = "(" + markings[pair_from] +")" + "(" + S + ")*" + "(" + markings[pair_to] + ")" + "|" + markings[tmp];
						}
						else
						{
							markings[tmp] = "(" + markings[pair_from] + ")" + "(" + S + ")*" + "(" + markings[pair_to] + ")";
						}
					}

				}
			}
			cout << "excluded: " << *i << endl;
			for(auto i = markings.begin(); i != markings.end(); i++)
			{
				cout << i->first.first << " " << i->first.second << ": " << i->second << endl;
			}
			cout << endl;
		}
	}
	pair<string, string> final("new_start", "new_end");
	auto res = markings[final];
	res.erase(remove(res.begin(), res.end(), '#'), res.end());
	cout << res << endl;
	return res;
}

