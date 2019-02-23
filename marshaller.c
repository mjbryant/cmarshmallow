#include <Python.h>

static PyObject *
get_value_from_object(PyObject *key, PyObject *obj, PyObject *default_obj)
{
    if (PyLong_CheckExact(key)) {
        // For int keys, all we can do is attempt a dict lookup
        PyObject* result;
        result = PyObject_GetItem(obj, key);
        if (result == NULL) {
            PyErr_Clear();
            return default_obj;
        } else {
            return result;
        }
    } else if (PyUnicode_CheckExact(key)) {
        PyObject *SEPARATOR = PyUnicode_FromString(".");
        PyObject* keys = PyUnicode_Split(key, SEPARATOR, -1);
        PyObject* prev_obj = obj;
        PyObject* next_obj;
        PyObject* iter = PyObject_GetIter(keys);
        PyObject* item;
        while ((item = PyIter_Next(iter))) {
            // Try dict access
            next_obj = PyObject_GetItem(prev_obj, item);
            if (next_obj == NULL) {
                PyErr_Clear();
                // If dict access fails, try getattr
                next_obj = PyObject_GetAttr(prev_obj, item);
                if (next_obj == NULL) {
                    // If getattr fails, return the default
                    PyErr_Clear();
                    return default_obj;
                }
            }
            prev_obj = next_obj;
        }
        return prev_obj;
    }

    Py_RETURN_NONE;
}

// Get the value of an attribute from an arbitrary python object
// If the key is an int, try to do a dict lookup on `obj`, otherwise default
// Otherwise, the key should be a string, and we should split the key on '.'
// and recursively get the value until we get the last part of the split.
// For example, if the key is 'a.b.c', we'd call:
//  get_value('b.c', get_value('a', obj))
static PyObject *
marshaller_get_value_from_object(PyObject *self, PyObject *args)
{
    PyObject* obj;
    PyObject* default_obj;
    PyObject* key;
    if (!PyArg_ParseTuple(args, "OOO", &key, &obj, &default_obj))
        return NULL;

    return get_value_from_object(key, obj, default_obj);
}

// Marshal a python object into a python dict.
// Marshalling involves taking in the obj to be marshalled and a map of
// attr_name -> field objects. For each (attr, field) pair, the marshaller
// attempts to pull the named attribute off obj, then calls field._serialize
// to get the result to include in the result dict.
static PyObject *
marshaller_marshal(PyObject *self, PyObject *args)
{
    PyObject *obj;
    PyObject *fields;
    int *many;
    if (!PyArg_ParseTuple(args, "OOp", &obj, &fields, &many))
        return NULL;

    PyObject *result = PyDict_New();
    // So I don't have to figure out how to pull default yet
    PyObject *sentinel_default = PyDict_New();

    PyObject *value;
    PyObject *serialized_value;

    PyObject *key, *field_obj;
    PyObject *field_serialize_func;
    Py_ssize_t pos = 0;

    while (PyDict_Next(fields, &pos, &key, &field_obj)) {
        // For each field to serialize, grab the value from the object
        value = get_value_from_object(key, obj, sentinel_default);
        // Serialize it according to the field
        field_serialize_func = PyObject_GetAttrString(field_obj, "_serialize");
        serialized_value = PyObject_CallFunctionObjArgs(
                field_serialize_func, value, key, obj, NULL);
        PyObject_SetItem(result, key, serialized_value);
    }

    return result;
}

static PyObject *
marshaller_call_object(PyObject *self, PyObject *args)
{
    PyObject *obj;
    PyObject *call_obj;
    if (!PyArg_ParseTuple(args, "OO", &obj, &call_obj))
        return NULL;
    PyObject *result = PyObject_CallFunctionObjArgs(obj, call_obj, NULL);
    return result;
}

static PyMethodDef MarshallerMethods[] = {
    {"marshal",  marshaller_marshal, METH_VARARGS, "Marshal"},
    {"get_value", marshaller_get_value_from_object, METH_VARARGS, "Get value"},
    {"call_object", marshaller_call_object, METH_VARARGS, "Call object"},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

static struct PyModuleDef marshaller = {
    PyModuleDef_HEAD_INIT,
    "marshaller",   /* name of module */
    NULL,           /* module documentation, may be NULL */
    -1,             /* size of per-interpreter state of the module,
                       or -1 if the module keeps state in global variables. */
    MarshallerMethods
};

PyMODINIT_FUNC
PyInit_marshaller(void)
{
    return PyModule_Create(&marshaller);
}
