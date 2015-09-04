#include "php_cassandra.h"
#include "util/collections.h"
#include "src/Cassandra/Collection.h"

zend_class_entry *cassandra_collection_ce = NULL;

int
php_cassandra_collection_add(cassandra_collection* collection, zval* object TSRMLS_DC)
{
  if (zend_hash_next_index_insert(&collection->values, (void*) &object, sizeof(zval*), NULL) == SUCCESS) {
    Z_ADDREF_P(object);
    return 1;
  }

  return 0;
}

static int
php_cassandra_collection_del(cassandra_collection* collection, ulong index)
{
  if (zend_hash_index_del(&collection->values, index) == SUCCESS)
    return 1;

  return 0;
}

static int
php_cassandra_collection_get(cassandra_collection* collection, ulong index, zval** zvalue)
{
  zval** value;

  if (zend_hash_index_find(&collection->values, index, (void**) &value) == SUCCESS) {
    *zvalue = *value;
    return 1;
  }

  return 0;
}

static int
php_cassandra_collection_find(cassandra_collection* collection, zval* object, long* index TSRMLS_DC)
{
  HashPointer ptr;
  zval**      current;
  zval        compare;
  ulong       idx;

  if (!php_cassandra_validate_object(object, collection->type TSRMLS_CC))
    return 0;

  zend_hash_get_pointer(&collection->values, &ptr);
  zend_hash_internal_pointer_reset(&collection->values);

  while (zend_hash_get_current_data(&collection->values, (void**) &current) == SUCCESS) {
    is_equal_function(&compare, object, *current TSRMLS_CC);
    if (Z_LVAL(compare) && zend_hash_get_current_key(&collection->values, NULL, &idx, 0) == HASH_KEY_IS_LONG) {
      *index = (long) idx;
      return 1;
    }

    zend_hash_move_forward(&collection->values);
  }


  zend_hash_set_pointer(&collection->values, &ptr);

  return 0;
}

static void
php_cassandra_collection_populate(cassandra_collection* collection, zval* array)
{
  HashPointer ptr;
  zval** current;

  zend_hash_get_pointer(&collection->values, &ptr);
  zend_hash_internal_pointer_reset(&collection->values);

  while (zend_hash_get_current_data(&collection->values, (void**) &current) == SUCCESS) {
    if (add_next_index_zval(array, *current) == SUCCESS)
      Z_ADDREF_PP(current);
    else
      break;

    zend_hash_move_forward(&collection->values);
  }

  zend_hash_set_pointer(&collection->values, &ptr);
}

/* {{{ Cassandra\Collection::__construct(string) */
PHP_METHOD(Collection, __construct)
{
  char *type;
  int type_len;
  cassandra_collection* collection;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &type, &type_len) == FAILURE) {
    return;
  }

  collection = (cassandra_collection*) zend_object_store_get_object(getThis() TSRMLS_CC);

  php_cassandra_value_type(type, &collection->type TSRMLS_CC);
}
/* }}} */

/* {{{ Cassandra\Collection::type() */
PHP_METHOD(Collection, type)
{
  cassandra_collection* collection = (cassandra_collection*) zend_object_store_get_object(getThis() TSRMLS_CC);

  RETURN_STRING(php_cassandra_type_name(collection->type), 1);
}
/* }}} */

/* {{{ Cassandra\Collection::values() */
PHP_METHOD(Collection, values)
{
  cassandra_collection* collection = NULL;
  array_init(return_value);
  collection = (cassandra_collection*) zend_object_store_get_object(getThis() TSRMLS_CC);
  php_cassandra_collection_populate(collection, return_value);
}
/* }}} */

/* {{{ Cassandra\Collection::add(mixed) */
PHP_METHOD(Collection, add)
{
  zval*** args;
  cassandra_collection* collection = NULL;
  int argc, i;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "+", &args, &argc) == FAILURE)
    return;

  collection = (cassandra_collection*) zend_object_store_get_object(getThis() TSRMLS_CC);

  for (i = 0; i < argc; i++) {
    if (Z_TYPE_P(*args[i]) == IS_NULL) {
      efree(args);
      zend_throw_exception_ex(cassandra_invalid_argument_exception_ce, 0 TSRMLS_CC,
                              "Invalid value: null is not supported inside collections");
      RETURN_FALSE;
    }

    if (!php_cassandra_validate_object(*args[i], collection->type TSRMLS_CC)) {
      efree(args);
      RETURN_FALSE;
    }
  }

  for (i = 0; i < argc; i++) {
    php_cassandra_collection_add(collection, *args[i] TSRMLS_CC);
  }

  efree(args);
  RETVAL_LONG(zend_hash_num_elements(&collection->values));
}
/* }}} */

/* {{{ Cassandra\Collection::get(int) */
PHP_METHOD(Collection, get)
{
  long key;
  cassandra_collection* collection = NULL;
  zval* value;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &key) == FAILURE)
    return;

  collection = (cassandra_collection*) zend_object_store_get_object(getThis() TSRMLS_CC);

  if (php_cassandra_collection_get(collection, (ulong) key, &value))
    RETURN_ZVAL(value, 1, 0);
}
/* }}} */

/* {{{ Cassandra\Collection::find(mixed) */
PHP_METHOD(Collection, find)
{
  zval* object;
  cassandra_collection* collection = NULL;
  long index;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &object) == FAILURE)
    return;

  collection = (cassandra_collection*) zend_object_store_get_object(getThis() TSRMLS_CC);

  if (php_cassandra_collection_find(collection, object, &index TSRMLS_CC))
    RETURN_LONG(index);
}
/* }}} */

/* {{{ Cassandra\Collection::count() */
PHP_METHOD(Collection, count)
{
  cassandra_collection* collection = (cassandra_collection*) zend_object_store_get_object(getThis() TSRMLS_CC);
  RETURN_LONG(zend_hash_num_elements(&collection->values));
}
/* }}} */

/* {{{ Cassandra\Collection::current() */
PHP_METHOD(Collection, current)
{
  zval** current;
  cassandra_collection* collection = (cassandra_collection*) zend_object_store_get_object(getThis() TSRMLS_CC);

  if (zend_hash_get_current_data(&collection->values, (void**) &current) == SUCCESS)
    RETURN_ZVAL(*current, 1, 0);
}
/* }}} */

/* {{{ Cassandra\Collection::key() */
PHP_METHOD(Collection, key)
{
  ulong index;
  cassandra_collection* collection = (cassandra_collection*) zend_object_store_get_object(getThis() TSRMLS_CC);

  if (zend_hash_get_current_key(&collection->values, NULL, &index, 0) == HASH_KEY_IS_LONG)
    RETURN_LONG(index);
}
/* }}} */

/* {{{ Cassandra\Collection::next() */
PHP_METHOD(Collection, next)
{
  cassandra_collection* collection = (cassandra_collection*) zend_object_store_get_object(getThis() TSRMLS_CC);
  zend_hash_move_forward(&collection->values);
}
/* }}} */

/* {{{ Cassandra\Collection::valid() */
PHP_METHOD(Collection, valid)
{
  cassandra_collection* collection = (cassandra_collection*) zend_object_store_get_object(getThis() TSRMLS_CC);
  RETURN_BOOL(zend_hash_has_more_elements(&collection->values) == SUCCESS);
}
/* }}} */

/* {{{ Cassandra\Collection::rewind() */
PHP_METHOD(Collection, rewind)
{
  cassandra_collection* collection = (cassandra_collection*) zend_object_store_get_object(getThis() TSRMLS_CC);
  zend_hash_internal_pointer_reset(&collection->values);
}
/* }}} */

/* {{{ Cassandra\Collection::remove(key) */
PHP_METHOD(Collection, remove)
{
  long index;
  cassandra_collection* collection = NULL;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &index) == FAILURE) {
    return;
  }

  collection = (cassandra_collection*) zend_object_store_get_object(getThis() TSRMLS_CC);

  if (php_cassandra_collection_del(collection, (ulong) index)) {
    RETURN_TRUE;
  }

  RETURN_FALSE;
}
/* }}} */

ZEND_BEGIN_ARG_INFO_EX(arginfo__construct, 0, ZEND_RETURN_VALUE, 1)
  ZEND_ARG_INFO(0, type)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_value, 0, ZEND_RETURN_VALUE, 1)
  ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_index, 0, ZEND_RETURN_VALUE, 1)
  ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_none, 0, ZEND_RETURN_VALUE, 0)
ZEND_END_ARG_INFO()

static zend_function_entry cassandra_collection_methods[] = {
  PHP_ME(Collection, __construct, arginfo__construct, ZEND_ACC_CTOR|ZEND_ACC_PUBLIC)
  PHP_ME(Collection, type, arginfo_none, ZEND_ACC_PUBLIC)
  PHP_ME(Collection, values, arginfo_none, ZEND_ACC_PUBLIC)
  PHP_ME(Collection, add, arginfo_value, ZEND_ACC_PUBLIC)
  PHP_ME(Collection, get, arginfo_index, ZEND_ACC_PUBLIC)
  PHP_ME(Collection, find, arginfo_value, ZEND_ACC_PUBLIC)
  /* Countable */
  PHP_ME(Collection, count, arginfo_none, ZEND_ACC_PUBLIC)
  /* Iterator */
  PHP_ME(Collection, current, arginfo_none, ZEND_ACC_PUBLIC)
  PHP_ME(Collection, key, arginfo_none, ZEND_ACC_PUBLIC)
  PHP_ME(Collection, next, arginfo_none, ZEND_ACC_PUBLIC)
  PHP_ME(Collection, valid, arginfo_none, ZEND_ACC_PUBLIC)
  PHP_ME(Collection, rewind, arginfo_none, ZEND_ACC_PUBLIC)
  PHP_ME(Collection, remove, arginfo_index, ZEND_ACC_PUBLIC)
  PHP_FE_END
};

static zend_object_handlers cassandra_collection_handlers;

static HashTable*
php_cassandra_collection_gc(zval *object, zval ***table, int *n TSRMLS_DC)
{
  *table = NULL;
  *n = 0;
  return zend_std_get_properties(object TSRMLS_CC);
}

static HashTable*
php_cassandra_collection_properties(zval *object TSRMLS_DC)
{
  zval* values;

  cassandra_collection* collection = (cassandra_collection*) zend_object_store_get_object(object TSRMLS_CC);
  HashTable*     props = zend_std_get_properties(object TSRMLS_CC);

  MAKE_STD_ZVAL(values);
  array_init(values);

  php_cassandra_collection_populate(collection, values);

  zend_hash_update(props, "values", sizeof("values"), &values, sizeof(zval), NULL);

  return props;
}

int zend_compare_symbol_tables_i(HashTable *ht1, HashTable *ht2 TSRMLS_DC);

static int
php_cassandra_collection_compare(zval *obj1, zval *obj2 TSRMLS_DC)
{
  cassandra_collection* collection1 = NULL;
  cassandra_collection* collection2 = NULL;

  if (Z_OBJCE_P(obj1) != Z_OBJCE_P(obj2))
    return 1; /* different classes */

  collection1 = (cassandra_collection*) zend_object_store_get_object(obj1 TSRMLS_CC);
  collection2 = (cassandra_collection*) zend_object_store_get_object(obj2 TSRMLS_CC);

  if (collection1->type != collection2->type)
    return 1;

  return zend_compare_symbol_tables_i(&collection1->values, &collection2->values TSRMLS_CC);
}

static void
php_cassandra_collection_free(void *object TSRMLS_DC)
{
  cassandra_collection* collection = (cassandra_collection*) object;

  zend_hash_destroy(&collection->values);
  zend_object_std_dtor(&collection->zval TSRMLS_CC);

  efree(collection);
}

static zend_object_value
php_cassandra_collection_new(zend_class_entry* class_type TSRMLS_DC)
{
  zend_object_value retval;
  cassandra_collection *collection;

  collection = (cassandra_collection*) emalloc(sizeof(cassandra_collection));
  memset(collection, 0, sizeof(cassandra_collection));

  zend_hash_init(&collection->values, 0, NULL, ZVAL_PTR_DTOR, 0);
  zend_object_std_init(&collection->zval, class_type TSRMLS_CC);
  object_properties_init(&collection->zval, class_type);

  retval.handle   = zend_objects_store_put(collection,
                      (zend_objects_store_dtor_t) zend_objects_destroy_object,
                      php_cassandra_collection_free, NULL TSRMLS_CC);
  retval.handlers = &cassandra_collection_handlers;

  return retval;
}

void cassandra_define_Collection(TSRMLS_D)
{
  zend_class_entry ce;

  INIT_CLASS_ENTRY(ce, "Cassandra\\Collection", cassandra_collection_methods);
  cassandra_collection_ce = zend_register_internal_class(&ce TSRMLS_CC);
  memcpy(&cassandra_collection_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
  cassandra_collection_handlers.get_properties  = php_cassandra_collection_properties;
#if PHP_VERSION_ID >= 50400
  cassandra_collection_handlers.get_gc          = php_cassandra_collection_gc;
#endif
  cassandra_collection_handlers.compare_objects = php_cassandra_collection_compare;
  cassandra_collection_ce->ce_flags |= ZEND_ACC_FINAL_CLASS;
  cassandra_collection_ce->create_object = php_cassandra_collection_new;
  zend_class_implements(cassandra_collection_ce TSRMLS_CC, 2, spl_ce_Countable, zend_ce_iterator);
}
