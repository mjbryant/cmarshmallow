from cmarshmallow import Schema, fields


class FooSerializer(Schema):
    _id = fields.Integer()
