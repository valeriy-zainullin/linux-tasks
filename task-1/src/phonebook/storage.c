#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/mutex.h>

#include "globals.h"
#include "phonebook/phonebook.h"

static bool item_used[PB_PHONEBOOK_SIZE] = {0};
static struct pb_user_data items[PB_PHONEBOOK_SIZE] = {0};

static bool find_item(const struct pb_user_data* ud, size_t* pos) {
	for (size_t i = 0; i < PB_PHONEBOOK_SIZE; ++i) {
		if (item_used[i] && pb_ud_cmp(&items[i], ud) == 0) {
			if (pos != NULL) {
				*pos = i;
			}

			return true;
		}
	}

	return false;
}

static bool find_space(size_t* pos) {
	for (size_t i = 0; i < PB_PHONEBOOK_SIZE; ++i) {
		if (!item_used[i]) {
			if (pos != NULL) {
				*pos = i;
			}

			return true;
		}
	}

	return false;
}

static void copy_pb(struct pb_user_data* dst_ud, const struct pb_user_data* src_ud) {
	memcpy(dst_ud->first_name, src_ud->first_name, sizeof(dst_ud->first_name));
	memcpy(dst_ud->last_name,  src_ud->last_name,  sizeof(dst_ud->last_name));
	memcpy(&dst_ud->age,       &src_ud->age,       sizeof(dst_ud->age));
	memcpy(dst_ud->telnum,     src_ud->telnum,     sizeof(dst_ud->telnum));
	memcpy(dst_ud->email,      src_ud->email,      sizeof(dst_ud->email));
}

bool pb_add(size_t* id, const struct pb_user_data* ud) {
	mutex_lock(&pb_list_mutex);

	bool has_item = find_item(ud, NULL);

	if (has_item) {
		mutex_unlock(&pb_list_mutex);
		return false;
	}

	size_t pos = 0;
	if (!find_space(&pos)) {
		mutex_unlock(&pb_list_mutex);
		return false;
	}

	copy_pb(&items[pos], ud);
	item_used[pos] = true;

	if (id != NULL) {
		*id = pos;
	}

	mutex_unlock(&pb_list_mutex);
	return true;
}

bool pb_get_by_id(size_t id, struct pb_user_data* ud) {
	mutex_lock(&pb_list_mutex);

	// pos === id.
	if (id >= PB_PHONEBOOK_SIZE || !item_used[id]) {
		mutex_unlock(&pb_list_mutex);
		return false;
	}

	copy_pb(ud, &items[id]);

	mutex_unlock(&pb_list_mutex);
	return true;

}

bool pb_get_geq_id_ud_with_last_name(size_t id_start, const char* last_name, size_t *id, struct pb_user_data* ud) {
	mutex_lock(&pb_list_mutex);

	for (size_t i = id_start; i < PB_PHONEBOOK_SIZE; ++i) {
		if (item_used[i] &&
			strncmp(
				items[i].last_name,
				last_name,
				sizeof(items[i].last_name) / sizeof(char)) == 0)
		{
			if (id != NULL) {
				*id = i;
			}

			if (ud != NULL) {
				*ud = items[i];
			}

			mutex_unlock(&pb_list_mutex);
			return true;
		}
	}

	mutex_unlock(&pb_list_mutex);
	return false;
}

bool pb_remove_by_id(size_t id) {
	mutex_lock(&pb_list_mutex);

	if (id >= PB_PHONEBOOK_SIZE || !item_used[id]) {
		mutex_unlock(&pb_list_mutex);
		return false;
	}

	item_used[id] = false;
	mutex_unlock(&pb_list_mutex);
	return true;
}

size_t pb_remove_by_last_name(const char* last_name) {
	mutex_lock(&pb_list_mutex);

	size_t num_deleted = 0;

	for (size_t i = 0; i < PB_PHONEBOOK_SIZE; ++i) {
		if (item_used[i] && strcmp(items[i].last_name, last_name) == 0) {
			item_used[i] = false;
			num_deleted += 1;
		}
	}

	mutex_unlock(&pb_list_mutex);

	return num_deleted;
}

#undef PHONEBOOK_SIZE
