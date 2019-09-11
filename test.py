from cmarshmallow import Schema, fields


class S(Schema):
    i = fields.Int()

result = S().dumps({'i': 'abc'})
print(result)
