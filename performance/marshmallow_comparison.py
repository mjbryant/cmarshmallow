"""
Compare the performance of cmarshmallow with the original marshmallow. The main
focus is on schema dumping.

TODO:
    * use some custom fields
"""
from __future__ import division, print_function, unicode_literals

import datetime
import gc
import timeit
from uuid import uuid4

import marshmallow
from marshmallow import fields

import cmarshmallow
from cmarshmallow import fields as cfields


class SmallerMarshmallow(marshmallow.Schema):
    x = fields.Int()
    y = fields.List(fields.Int)
    z = fields.Str()


class SmallerCMarshmallow(cmarshmallow.Schema):
    x = cfields.Int()
    y = cfields.List(cfields.Int)
    z = cfields.Str()


class LargeCMarshmallow(cmarshmallow.Schema):
    uid = cfields.UUID()
    uid2 = cfields.UUID()
    links = cfields.List(cfields.UUID)
    description = cfields.Str()
    alt_description = cfields.Str()
    version = cfields.Int()
    version_name = cfields.Str()
    email = cfields.Email()
    height = cfields.Int()
    width = cfields.Int()
    thumb_height = cfields.Int()
    thumb_width = cfields.Int()
    name = cfields.Str()
    alt_name = cfields.Str()
    third_name = cfields.Str()
    page = cfields.Int()
    size = cfields.Int()
    rotation = cfields.Int()
    status = cfields.Str()
    other_things = cfields.List(cfields.Str)
    nested = cfields.Nested(SmallerCMarshmallow)
    urls = cfields.Dict()
    user_input = cfields.Int()
    created_at = cfields.DateTime()
    created_by = cfields.Str()
    updated_at = cfields.DateTime()
    updated_by = cfields.Str()
    deleted_at = cfields.DateTime()
    deleted_by = cfields.Str()


class LargeMarshmallow(marshmallow.Schema):
    uid = fields.UUID()
    uid2 = fields.UUID()
    links = fields.List(fields.UUID)
    description = fields.Str()
    alt_description = fields.Str()
    version = fields.Int()
    version_name = fields.Str()
    email = fields.Email()
    height = fields.Int()
    width = fields.Int()
    thumb_height = fields.Int()
    thumb_width = fields.Int()
    name = fields.Str()
    alt_name = fields.Str()
    third_name = fields.Str()
    page = fields.Int()
    size = fields.Int()
    rotation = fields.Int()
    status = fields.Str()
    other_things = fields.List(fields.Str)
    nested = fields.Nested(SmallerMarshmallow)
    urls = fields.Dict()
    user_input = fields.Int()
    created_at = fields.DateTime()
    created_by = fields.Str()
    updated_at = fields.DateTime()
    updated_by = fields.Str()
    deleted_at = fields.DateTime()
    deleted_by = fields.Str()


class Large:
    def __init__(self, **kwargs):
        for k, v in kwargs.items():
            setattr(self, k, v)


def _make_dt():
    return datetime.datetime.now()


def _make_large():
    return Large(
        uid=uuid4(), uid2=uuid4(), links=[uuid4(), uuid4(), uuid4()],
        description='Lorem ipsum whatever shmipsom', alt_description='a',
        version=1241, version_name='Pig Slap', email='whatever@hi.com',
        height=5280, width=1, thumb_height=100, thumb_width=100,
        name='Turkish Delight', alt_name='Witch', third_name='Edmund',
        page=15, size=1500, rotation=90, status='fin',
        other_things=['hi', 'no', 'okay', 'sure'],
        nested={'x': 1, 'y': [1, 2, 3], 'z': 'tacos'},
        urls={'horcruxes': 'two'}, user_input=12,
        created_at=_make_dt(), created_by='123',
        updated_at=_make_dt(), updated_by='123',
        deleted_at=_make_dt(), deleted_by='456',
    )


def compare_large_schema():
    M = LargeMarshmallow(many=True)
    C = LargeCMarshmallow(many=True)
    objs = [_make_large() for _ in range(100)]
    iterations = 100
    gc.collect()
    # Take the fastest run of 1000 dumps and get the best time for one dump
    best_m = min(timeit.repeat(
        lambda: M.dumps(objs),
        'gc.enable()',
        number=iterations,
        repeat=3,
    )) / iterations
    best_c = min(timeit.repeat(
        lambda: C.dumps(objs),
        'gc.enable()',
        number=iterations,
        repeat=3,
    )) / iterations
    print(
        'Marshmallow (large): {}s\n'.format(round(best_m, 4)),
        'Cmarshmallow (large): {}s'.format(round(best_c, 4)),
    )


def compare_custom_fields():
    pass


def compare_many():
    pass


if __name__ == '__main__':
    compare_large_schema()
