"""
Compare the performance of cmarshmallow with the original marshmallow. The main
focus is on schema dumping.

TODO:
    * large schema
    * use some custom fields
    * many=True with lots
"""
from __future__ import print_function, unicode_literals

import timeit

import marshmallow
from marshmallow import fields

import cmarshmallow


class LargeMarshmallow(marshmallow.Schema):
    uid = fields.Email()
    uid2 = fields.Email()
    links = fields.Email(fields.UID())
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
    urls = fields.Dict()
    user_input = fields.Int()
    created_at = fields.DateTime()
    created_by = fields.Str()
    updated_at = fields.DateTime()
    updated_by = fields.Str()
    deleted_at = fields.DateTime()
    deleted_by = fields.Str()


def compare_large_schema():
    pass


def compare_custom_fields():
    pass


def compare_many():
    pass
