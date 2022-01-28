#include <string>
#include <vector>
#include <utility>
#include <chrono>
#include <iostream>
#include <fstream>
#include <algorithm>

using namespace std;
using namespace chrono;

using write_sequence = vector<string>;

using test_pair = pair<uint64_t, string>;
using modify_sequence = vector<test_pair>;
using read_sequence = vector<test_pair>;

ifstream &operator>>( ifstream &_is, test_pair &_value ) {
	_is >> _value.first;
	_is >> _value.second;

	return _is;
}

template<typename S>
S get_sequence( const string &_file_name ) {
	ifstream infile(_file_name);
	S result;

	typename S::value_type item;

	while( infile >> item ) {
		result.emplace_back(move(item));
	}

	return result;
}

class storage {
	class node {
		node *left;
		node *right;
		string data;
		size_t cnt;
		unsigned char h;
		std::string nullstr;

		static unsigned char height( node *p ) {
			return p ? p->h : 0;
		}

		static size_t count( node *p ) {
			return p ? p->cnt : 0;
		}

		int bfactor() {
			return height(right) - height(left);
		}

		void fixheight() {
			unsigned char hl = height(left);
			unsigned char hr = height(right);
			size_t cl = count(left);
			size_t cr = count(right);
			h = (hl > hr ? hl : hr) + 1;
			cnt = cl + cr + 1;
		}

		node *rotateright() {
			node *q = left;
			left = q->right;
			q->right = this;
			fixheight();
			q->fixheight();
			return q;
		}

		node *rotateleft() {
			node *p = right;
			right = p->left;
			p->left = this;
			fixheight();
			p->fixheight();
			return p;
		}

		node *balance() {
			fixheight();
			if( bfactor() == 2 ) {
				if( right->bfactor() < 0 )
					right = right->rotateright();
				return rotateleft();
			}
			if( bfactor() == -2 ) {
				if( left->bfactor() > 0 )
					left = left->rotateleft();
				return rotateright();
			}
			return this;
		}

		node *findmin( node *p )
		{
			return p->left ? findmin(p->left) : p;
		}

		node *removemin( node *p )
		{
			if( p->left == nullptr )
				return p->right;
			p->left = removemin(p->left);
			return p->balance();
		}

		node *getNode( uint64_t _index ) {
			if( cnt == 1 ) {
				return this;
			} else if( cnt < 4 ) {
				if( !left ) {
					_index++;
				}
				switch( _index ) {
					case 0:
						return left;
					case 1:
						return this;
					case 2:
						return right;
				}
			}

			if( left->cnt >= _index ) {
				if( left->cnt > _index ) {
					return left->getNode(_index);
				} else {
					return this;
				}
			} else {
				return right->getNode(_index - left->cnt - 1);
			}
		}

		node *removeNode( uint64_t _index ) {
			if( cnt == 1 ) {
				delete this;
				return nullptr;
			} else if( cnt < 4 ) {
				if( !left ) {
					_index++;
				}
				switch( _index ) {
					case 0:
						delete left;
						left = nullptr;
						return balance();
					case 1: {
						node * n = left ? left : right;
						if (cnt == 3) {
							left->right = right;
						}
						delete this;
						return n->balance();
					}
					case 2:
						delete right;
						right = nullptr;
						return balance();
				}
			}

			if( left->cnt >= _index ) {
				if( left->cnt > _index ) {
					left = left->removeNode(_index);
					return balance();
				} else {
					node *q = left;
					node *r = right;
					delete this;
					if( !r ) return q;
					node *min = findmin(r);
					min->right = removemin(r);
					min->left = q;
					return min->balance();
				}
			} else {
				right = right->removeNode(_index - left->cnt - 1);
				return balance();
			}
		}

	public:
		node( string s ) {
			left = nullptr;
			right = nullptr;
			data = s;
			cnt = 1;
			h = 1;
		}

		node *insert( const string &_str )
		{
			if( _str < data ) {
				if( left ) {
					left = left->insert(_str);
				} else {
					left = new node(_str);
				}
			} else {
				if( right ) {
					right = right->insert(_str);
				} else {
					right = new node(_str);
				}
			}
			return balance();
		}

		const string &getString( uint64_t _index ) {
			if( cnt <= _index ) {
				return nullstr;
			}
			return getNode(_index)->data;
		}

		node *remove( uint64_t _index ) {
			if( cnt <= _index ) {
				return nullptr;
			}
			return removeNode(_index);
		}
	};

	node *root;
	std::string nullstr;
public:
	storage() : root(nullptr) {};

	void insert( const string &_str ) {
		if( root ) {
			root = root->insert(_str);
		} else {
			root = new node(_str);
		}
	}

	void erase( uint64_t _index ) {
		if( root ) {
			root = root->remove(_index);
		}
	}

	const string &get( uint64_t _index ) {
		if( root ) {
			return root->getString(_index);
		} else {
			return nullstr;
		}
	}
};

int main() {
	write_sequence write = get_sequence<write_sequence>("write.txt");
	modify_sequence modify = get_sequence<modify_sequence>("modify.txt");
	read_sequence read = get_sequence<read_sequence>("read.txt");

	storage st;

	for( const string &item: write ) {
		st.insert(item);
	}

	uint64_t progress = 0;
	uint64_t percent = modify.size() / 100;

	time_point<system_clock> time;
	nanoseconds total_time(0);

	modify_sequence::const_iterator mitr = modify.begin();
	read_sequence::const_iterator ritr = read.begin();

	for( ; mitr != modify.end() && ritr != read.end(); ++mitr, ++ritr ) {
		time = system_clock::now();
		st.erase(mitr->first);
		st.insert(mitr->second);
		const string &str = st.get(ritr->first);
		total_time += system_clock::now() - time;

		if( ritr->second != str ) {
			cout << "test failed" << endl;
			return 1;
		}

		if( ++progress % (5 * percent) == 0 ) {
			cout << "time: " << duration_cast<milliseconds>(total_time).count()
			     << "ms progress: " << progress << " / " << modify.size() << "\n";
		}
	}

	return 0;
}
