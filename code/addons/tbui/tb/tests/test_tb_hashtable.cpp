// ================================================================================
// ==      This file is a part of Turbo Badger. (C) 2011-2016, Emil Segerås      ==
// ==                     See tb_core.h for more information.                    ==
// ================================================================================

#include "tb_test.h"
#include "tb_hashtable.h"

#ifdef TB_UNIT_TESTING

using namespace tb;

TB_TEST_GROUP(tb_hashtable)
{
	class Apple
	{
	public:
		Apple(int id) : id(id) { total_apple_count++; }
		~Apple() { total_apple_count--; }

		int id;
		static int total_apple_count;
	};
	int Apple::total_apple_count = 0;

	TBHashTableOf<Apple> map;

	TB_TEST(simple)
	{
		map.Add(1, new Apple(1));
		map.Add(2, new Apple(2));
		TB_VERIFY(map.GetNumItems() == 2);
		TB_VERIFY(map.Get(1)->id == 1);
		TB_VERIFY(map.Get(2)->id == 2);
		TB_VERIFY(map.Remove(1)->id == 1);
		TB_VERIFY(map.Remove(2)->id == 2);
		TB_VERIFY(map.GetNumItems() == 0);
	}

	TB_TEST(autodelete)
	{
		// Check that the apples really are destroyed.
		int old_total_apple_count = Apple::total_apple_count;
		// Scope for TBLinkListAutoDeleteOf
		{
			TBHashTableAutoDeleteOf<Apple> autodelete_list;
			autodelete_list.Add(1, new Apple(1));
			autodelete_list.Add(2, new Apple(2));
			TB_VERIFY(Apple::total_apple_count == old_total_apple_count + 2);
		}
		TB_VERIFY(Apple::total_apple_count == old_total_apple_count);
	}
}

#endif // TB_UNIT_TESTING
