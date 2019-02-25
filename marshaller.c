#include <Python.h>

/**
 * Replacement for marshmallow's Marshaller that will hopefully greatly speed
 * up dumping objects.
 *
 * Things left to do, or at least research:
 * - Reference counting
 * - Memory management
 * - Error handling
 */

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
        // Low-priority: can we rewrite this using Daniel's approach to be faster?
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

    // This should probably be an error or NULL or something.
    Py_RETURN_NONE;
}

// Marshal a single python object into a single python dict
static PyObject *
marshal_one(PyObject *obj, PyObject *fields)
{
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

    if (many) {
        Py_ssize_t length = PyList_Size(obj);
        PyObject *ret = PyList_New(length);
        for (Py_ssize_t index = 0; index < length; index++) {
            PyList_SetItem(ret, index, marshal_one(PyList_GetItem(obj, index), fields));
        }
        return ret;
    } else {
        return marshal_one(obj, fields);
    }
}

static PyMethodDef MarshallerMethods[] = {
    {"marshal",  marshaller_marshal, METH_VARARGS, "Marshal"},
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
