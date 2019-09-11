#include <Python.h>

/**
 * Replacement for marshmallow's Marshaller that will hopefully greatly speed
 * up dumping objects.
 *
 * Things left to do, or at least research:
 * - Reference counting
 * - Memory management
 * - Error handling
 * - Handle _CHECK_ATTRIBUTE on certain field types
 * - Handle `attribute` in get_value_from_object
 */

#define noop

// Just attempt to get the attribute specified by `key` off of the object. If
// the attribute is missing, return default_obj instead.
// This function should not raise an error. If the attribute does not exist,
// we return the default object.
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
marshal_one(
        PyObject *obj, PyObject *fields, PyObject *validation_error,
        const char *prefix, PyObject *all_errors, Py_ssize_t index
)
{
    PyObject *result = PyDict_New();
    PyObject *errors = NULL;
    // So I don't have to figure out how to pull default yet
    PyObject *sentinel_default = PyDict_New();

    PyObject *value;
    PyObject *serialized_value;

    PyObject *key, *field_obj;
    PyObject *field_serialize_func;
    Py_ssize_t pos = 0;

    int use_prefix = 0;
    if (prefix[0] != '\0')
        use_prefix = 1;

    while (PyDict_Next(fields, &pos, &key, &field_obj)) {
        PyObject * load_only = PyObject_GetAttrString(field_obj, "load_only");
        if (!PyObject_IsTrue(load_only)) {
            // For each field to serialize, grab the value from the object
            value = get_value_from_object(key, obj, sentinel_default);
            // Serialize it according to the field
            field_serialize_func = PyObject_GetAttrString(field_obj, "_serialize");
            // This function can raise a ValidationError if serialization fails.
            // There are some fields that do not validate (e.g. String), so
            // maybe we can optimize there a bit.
            serialized_value = PyObject_CallFunctionObjArgs(
                    field_serialize_func, value, key, obj, NULL);
            if (serialized_value == NULL) {
                // If the function raises a ValidationError, then we drop into
                // this block. We want to store a bunch of information that makes
                // it easier for the user to tell which fields failed and why.
                // The actual fields here are a bit convoluted, and depend on
                // attributes of the ValidationError class and their type :-/
                // What we want to end up with is a dict of field_name (key)
                // to a dictionary of validation errors. For example, if field
                // 'i' isn't a valid integer, you get the error dict
                // {'i': ['Not a valid integer.]}.
                if (PyErr_ExceptionMatches(validation_error)) {
                    if (errors == NULL) {
                        errors = PyDict_New();
                    }
                    PyObject *err_type, *err_value, *traceback, *messages;
                    PyErr_Fetch(&err_type, &err_value, &traceback);
                    messages = PyObject_GetAttrString(err_value, "messages");
                    // It's unclear the different use cases that the existing
                    // error handling code is considering, so let's just store
                    // the messages for this ValidationError in the errors dict.
                    PyDict_SetItem(errors, key, messages);
                    PyErr_Clear();
                }
            } else {
                // key should be
                // ''.join([prefix or '', field_obj.dump_to or key]), but I can't
                // figure out how to append unicode objects yet
                if (use_prefix) {
                    return NULL;
                }

                PyDict_SetItem(result, key, serialized_value);
            }
        }
    }
    if (errors != NULL) {
        // Why is this showing up as marshal module object and not a dict like I want it to
        PyDict_SetItem(all_errors, PyLong_FromSsize_t(index), errors);
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
    // We shouldn't need to change reference counts to either of these incoming
    // objects. This C extension will always be called from Python, which will
    // guarantee that the calling Python code always maintains a reference to
    // these objects, so we can be sure they won't be deallocated.
    PyObject *obj;
    PyObject *fields;
    int many;
    PyObject *validation_error;
    const char *prefix;
    // TODO We can use O! to ensure that an arbitrary python object is the right type
    if (!PyArg_ParseTuple(args, "OOpOs",
                &obj, &fields, &many, &validation_error, &prefix))
        return NULL;

    // dict of index (for many=True) -> errors_dict
    PyObject *errors = PyDict_New();
    if (many) {
        // TODO should break the fields dict into C objects so we already have
        // the following properties pulled off them:
        // - load_only
        // - name of where to store the value (prefix + key)
        // - serialization function
        // - and so we don't have to use a python dict iterator to iterate
        //   through the values, since that's invariably slower than a bare C
        //   iterator.
        Py_ssize_t length = PyList_Size(obj);
        PyObject *ret = PyList_New(length);
        for (Py_ssize_t index = 0; index < length; index++) {
            // TODO GetItem returns a borrowed reference. What does this mean?
            // SetItem shouldn't need to incref, since it steals the existing
            // reference to the newly created marshalled dict.
            PyList_SetItem(ret, index,
                    marshal_one(
                        PyList_GetItem(obj, index),
                        fields,
                        validation_error,
                        prefix,
                        errors,
                        index));
        }
        // TODO check if we need to decref ret, since there's no longer a reference
        // to it from C code.
        return Py_BuildValue("(OO)", ret, errors);
    } else {
        PyObject *ret = marshal_one(
                obj,
                fields,
                validation_error,
                prefix,
                errors,
                0);
        return Py_BuildValue("(OO)", ret, errors);
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
