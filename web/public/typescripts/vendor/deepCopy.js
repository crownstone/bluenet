/* This file is part of OWL JavaScript Utilities.

 OWL JavaScript Utilities is free software: you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public License
 as published by the Free Software Foundation, either version 3 of
 the License, or (at your option) any later version.

 OWL JavaScript Utilities is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with OWL JavaScript Utilities.  If not, see
 <http://www.gnu.org/licenses/>.
 */

owl = (function() {

    // the re-usable constructor function used by clone().
    function Clone() {}

    // clone objects, skip other types.
    function clone(target) {
        if ( typeof target == 'object' ) {
            Clone.prototype = target;
            return new Clone();
        } else {
            return target;
        }
    }


    // Shallow Copy
    function copy(target) {
        if (typeof target !== 'object' ) {
            return target;  // non-object have value sematics, so target is already a copy.
        } else {
            var value = target.valueOf();
            if (target != value) {
                // the object is a standard object wrapper for a native type, say String.
                // we can make a copy by instantiating a new object around the value.
                return new target.constructor(value);
            } else {
                // ok, we have a normal object. If possible, we'll clone the original's prototype
                // (not the original) to get an empty object with the same prototype chain as
                // the original.  If just copy the instance properties.  Otherwise, we have to
                // copy the whole thing, property-by-property.
                if ( target instanceof target.constructor && target.constructor !== Object ) {
                    var c = clone(target.constructor.prototype);

                    // give the copy all the instance properties of target.  It has the same
                    // prototype as target, so inherited properties are already there.
                    for ( var property in target) {
                        if (target.hasOwnProperty(property)) {
                            c[property] = target[property];
                        }
                    }
                } else {
                    var c = {};
                    for ( var property in target ) c[property] = target[property];
                }

                return c;
            }
        }
    }

    // Deep Copy
    var deepCopiers = [];

    function DeepCopier(config) {
        for ( var key in config ) this[key] = config[key];
    }
    DeepCopier.prototype = {
        constructor: DeepCopier,

        // determines if this DeepCopier can handle the given object.
        canCopy: function(source) { return false; },

        // starts the deep copying process by creating the copy object.  You
        // can initialize any properties you want, but you can't call recursively
        // into the DeeopCopyAlgorithm.
        create: function(source) { },

        // Completes the deep copy of the source object by populating any properties
        // that need to be recursively deep copied.  You can do this by using the
        // provided deepCopyAlgorithm instance's deepCopy() method.  This will handle
        // cyclic references for objects already deepCopied, including the source object
        // itself.  The "result" passed in is the object returned from create().
        populate: function(deepCopyAlgorithm, source, result) {}
    };

    function DeepCopyAlgorithm() {
        // copiedObjects keeps track of objects already copied by this
        // deepCopy operation, so we can correctly handle cyclic references.
        this.copiedObjects = [];
        thisPass = this;
        this.recursiveDeepCopy = function(source) {
            return thisPass.deepCopy(source);
        }
        this.depth = 0;
    }
    DeepCopyAlgorithm.prototype = {
        constructor: DeepCopyAlgorithm,

        maxDepth: 256,

        // add an object to the cache.  No attempt is made to filter duplicates;
        // we always check getCachedResult() before calling it.
        cacheResult: function(source, result) {
            this.copiedObjects.push([source, result]);
        },

        // Returns the cached copy of a given object, or undefined if it's an
        // object we haven't seen before.
        getCachedResult: function(source) {
            var copiedObjects = this.copiedObjects;
            var length = copiedObjects.length;
            for ( var i=0; i<length; i++ ) {
                if ( copiedObjects[i][0] === source ) {
                    return copiedObjects[i][1];
                }
            }
            return undefined;
        },

        // deepCopy handles the simple cases itself: non-objects and object's we've seen before.
        // For complex cases, it first identifies an appropriate DeepCopier, then calls
        // applyDeepCopier() to delegate the details of copying the object to that DeepCopier.
        deepCopy: function(source) {
            // null is a special case: it's the only value of type 'object' without properties.
            if ( source === null ) return null;

            // All non-objects use value semantics and don't need explict copying.
            if ( typeof source !== 'object' ) return source;

            var cachedResult = this.getCachedResult(source);

            // we've already seen this object during this deep copy operation
            // so can immediately return the result.  This preserves the cyclic
            // reference structure and protects us from infinite recursion.
            if ( cachedResult ) return cachedResult;

            // objects may need special handling depending on their class.  There is
            // a class of handlers call "DeepCopiers"  that know how to copy certain
            // objects.  There is also a final, generic deep copier that can handle any object.
            for ( var i=0; i<deepCopiers.length; i++ ) {
                var deepCopier = deepCopiers[i];
                if ( deepCopier.canCopy(source) ) {
                    return this.applyDeepCopier(deepCopier, source);
                }
            }
            // the generic copier can handle anything, so we should never reach this line.
            throw new Error("no DeepCopier is able to copy " + source);
        },

        // once we've identified which DeepCopier to use, we need to call it in a very
        // particular order: create, cache, populate.  This is the key to detecting cycles.
        // We also keep track of recursion depth when calling the potentially recursive
        // populate(): this is a fail-fast to prevent an infinite loop from consuming all
        // available memory and crashing or slowing down the browser.
        applyDeepCopier: function(deepCopier, source) {
            // Start by creating a stub object that represents the copy.
            var result = deepCopier.create(source);

            // we now know the deep copy of source should always be result, so if we encounter
            // source again during this deep copy we can immediately use result instead of
            // descending into it recursively.
            this.cacheResult(source, result);

            // only DeepCopier::populate() can recursively deep copy.  So, to keep track
            // of recursion depth, we increment this shared counter before calling it,
            // and decrement it afterwards.
            this.depth++;
            if ( this.depth > this.maxDepth ) {
                throw new Error("Exceeded max recursion depth in deep copy.");
            }

            // It's now safe to let the deepCopier recursively deep copy its properties.
            deepCopier.populate(this.recursiveDeepCopy, source, result);

            this.depth--;

            return result;
        }
    };

    // entry point for deep copy.
    //   source is the object to be deep copied.
    //   maxDepth is an optional recursion limit. Defaults to 256.
    function deepCopy(source, maxDepth) {
        var deepCopyAlgorithm = new DeepCopyAlgorithm();
        if ( maxDepth ) deepCopyAlgorithm.maxDepth = maxDepth;
        return deepCopyAlgorithm.deepCopy(source);
    }

    // publicly expose the DeepCopier class.
    deepCopy.DeepCopier = DeepCopier;

    // publicly expose the list of deepCopiers.
    deepCopy.deepCopiers = deepCopiers;

    // make deepCopy() extensible by allowing others to
    // register their own custom DeepCopiers.
    deepCopy.register = function(deepCopier) {
        if ( !(deepCopier instanceof DeepCopier) ) {
            deepCopier = new DeepCopier(deepCopier);
        }
        deepCopiers.unshift(deepCopier);
    }

    // Generic Object copier
    // the ultimate fallback DeepCopier, which tries to handle the generic case.  This
    // should work for base Objects and many user-defined classes.
    deepCopy.register({
        canCopy: function(source) { return true; },

        create: function(source) {
            if ( source instanceof source.constructor ) {
                return clone(source.constructor.prototype);
            } else {
                return {};
            }
        },

        populate: function(deepCopy, source, result) {
            for ( var key in source ) {
                if ( source.hasOwnProperty(key) ) {
                    result[key] = deepCopy(source[key]);
                }
            }
            return result;
        }
    });

    // Array copier
    deepCopy.register({
        canCopy: function(source) {
            return ( source instanceof Array );
        },

        create: function(source) {
            return new source.constructor();
        },

        populate: function(deepCopy, source, result) {
            for ( var i=0; i<source.length; i++) {
                result.push( deepCopy(source[i]) );
            }
            return result;
        }
    });

    // Date copier
    deepCopy.register({
        canCopy: function(source) {
            return ( source instanceof Date );
        },

        create: function(source) {
            return new Date(source);
        }
    });

    // HTML DOM Node

    // utility function to detect Nodes.  In particular, we're looking
    // for the cloneNode method.  The global document is also defined to
    // be a Node, but is a special case in many ways.
    function isNode(source) {
        if ( window.Node ) {
            return source instanceof Node;
        } else {
            // the document is a special Node and doesn't have many of
            // the common properties so we use an identity check instead.
            if ( source === document ) return true;
            return (
                typeof source.nodeType === 'number' &&
                    source.attributes &&
                    source.childNodes &&
                    source.cloneNode
                );
        }
    }

    // Node copier
    deepCopy.register({
        canCopy: function(source) { return isNode(source); },

        create: function(source) {
            // there can only be one (document).
            if ( source === document ) return document;

            // start with a shallow copy.  We'll handle the deep copy of
            // its children ourselves.
            return source.cloneNode(false);
        },

        populate: function(deepCopy, source, result) {
            // we're not copying the global document, so don't have to populate it either.
            if ( source === document ) return document;

            // if this Node has children, deep copy them one-by-one.
            if ( source.childNodes && source.childNodes.length ) {
                for ( var i=0; i<source.childNodes.length; i++ ) {
                    var childCopy = deepCopy(source.childNodes[i]);
                    result.appendChild(childCopy);
                }
            }
        }
    });

    return {
        DeepCopyAlgorithm: DeepCopyAlgorithm,
        copy: copy,
        clone: clone,
        deepCopy: deepCopy
    };
})();