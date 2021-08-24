#include <iostream>
#include <vector>
#include <random>
#include "utree.h"

using namespace std;

#define NUM_RECORDS 1000000

void shuffle(std::vector<uint64_t>& keys) {
	uint64_t i, j, times = keys.size()/2;
	for (uint64_t k = 0; k < times; k++)
	{
		i = rand() % keys.size();
		j = rand() % keys.size();
		std::iter_swap(keys.begin() + i, keys.begin() + j);
	}
}

int main()
{
	auto x = time(NULL);
	cout << "Time: " << x << endl;
	srand(x);
	vector<uint64_t> keys;
	keys.reserve(NUM_RECORDS);
	for (int i = 1; i <= NUM_RECORDS; i++)
		keys.push_back(i);

	shuffle(keys);

	btree utree;

	for (auto key : keys)
		utree.insert(key, (char*)key);

	auto cur = utree.list_head->next;
	uint64_t prev_key = 0;
	uint64_t count = 0;
	while(cur)
	{
		count ++;
		if (cur->key < prev_key)
			printf("Order violation! Current key: %lu, Previous key: %lu\n", cur->key, prev_key);
		prev_key = cur->key;
		cur = cur->next;
	}
	printf("%lu records scanned\n", count);


	for (auto key : keys)
    {
        if (utree.search(key) == NULL)
            printf("Key not found! %lu\n", key);
    }
    printf("Insert & search test passed, now delete\n\n");


    shuffle(keys);
    for (auto key : keys)
    {
        utree.remove(key);
    }
    printf("All keys deleted!\n");

    return 0;

}