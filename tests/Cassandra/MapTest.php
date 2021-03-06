<?php

namespace Cassandra;

use Cassandra\Type;

/**
 * @requires extension cassandra
 */
class MapTest extends \PHPUnit_Framework_TestCase
{
    public function testSupportsKeyBasedAccess()
    {
        $map = Type::map(Type::varint(), Type::varchar())->create();
        $this->assertEquals(0, count($map));
        $map->set(new Varint('123'), 'value');
        $this->assertEquals(1, count($map));
        $this->assertTrue($map->has(new Varint('123')));
        $this->assertEquals('value', $map->get(new Varint('123')));
        $map->set(new Varint('123'), 'another value');
        $this->assertEquals(1, count($map));
        $this->assertEquals('another value', $map->get(new Varint('123')));
    }

    /**
     * @expectedException         InvalidArgumentException
     * @expectedExceptionMessage  Unsupported type 'custom type'
     */
    public function testSupportsOnlyCassandraTypesForKeys()
    {
        new Map('custom type', \Cassandra::TYPE_VARINT);
    }

    /**
     * @expectedException         InvalidArgumentException
     * @expectedExceptionMessage  Unsupported type 'another custom type'
     */
    public function testSupportsOnlyCassandraTypesForValues()
    {
        new Map(\Cassandra::TYPE_VARINT, 'another custom type');
    }

    /**
     * @expectedException         InvalidArgumentException
     * @expectedExceptionMessage  Invalid value: null is not supported inside maps
     */
    public function testSupportsNullValues()
    {
        $map = new Map(\Cassandra::TYPE_VARCHAR, \Cassandra::TYPE_VARCHAR);
        $map->set("test", null);
    }
}
