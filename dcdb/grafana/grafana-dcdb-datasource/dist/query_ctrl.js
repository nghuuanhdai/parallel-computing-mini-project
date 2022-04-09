'use strict';

System.register(['app/plugins/sdk', './css/query-editor.css!'], function (_export, _context) {
  "use strict";

  var QueryCtrl, _createClass, GenericDatasourceQueryCtrl;

  function _classCallCheck(instance, Constructor) {
    if (!(instance instanceof Constructor)) {
      throw new TypeError("Cannot call a class as a function");
    }
  }

  function _possibleConstructorReturn(self, call) {
    if (!self) {
      throw new ReferenceError("this hasn't been initialised - super() hasn't been called");
    }

    return call && (typeof call === "object" || typeof call === "function") ? call : self;
  }

  function _inherits(subClass, superClass) {
    if (typeof superClass !== "function" && superClass !== null) {
      throw new TypeError("Super expression must either be null or a function, not " + typeof superClass);
    }

    subClass.prototype = Object.create(superClass && superClass.prototype, {
      constructor: {
        value: subClass,
        enumerable: false,
        writable: true,
        configurable: true
      }
    });
    if (superClass) Object.setPrototypeOf ? Object.setPrototypeOf(subClass, superClass) : subClass.__proto__ = superClass;
  }

  return {
    setters: [function (_appPluginsSdk) {
      QueryCtrl = _appPluginsSdk.QueryCtrl;
    }, function (_cssQueryEditorCss) {}],
    execute: function () {
      _createClass = function () {
        function defineProperties(target, props) {
          for (var i = 0; i < props.length; i++) {
            var descriptor = props[i];
            descriptor.enumerable = descriptor.enumerable || false;
            descriptor.configurable = true;
            if ("value" in descriptor) descriptor.writable = true;
            Object.defineProperty(target, descriptor.key, descriptor);
          }
        }

        return function (Constructor, protoProps, staticProps) {
          if (protoProps) defineProperties(Constructor.prototype, protoProps);
          if (staticProps) defineProperties(Constructor, staticProps);
          return Constructor;
        };
      }();

      _export('GenericDatasourceQueryCtrl', GenericDatasourceQueryCtrl = function (_QueryCtrl) {
        _inherits(GenericDatasourceQueryCtrl, _QueryCtrl);

        function GenericDatasourceQueryCtrl($scope, $injector) {
          _classCallCheck(this, GenericDatasourceQueryCtrl);

          var _this = _possibleConstructorReturn(this, (GenericDatasourceQueryCtrl.__proto__ || Object.getPrototypeOf(GenericDatasourceQueryCtrl)).call(this, $scope, $injector));

          _this.scope = $scope;
          _this.target.levels = _this.target.levels || ['select level'];

          //Sensor dropdown menu values
          _this.target.sensor = _this.target.sensor || 'select sensor';
          _this.datasource.getMaxLevels();

          return _this;
        }

        //Add a new level to the array.


        _createClass(GenericDatasourceQueryCtrl, [{
          key: 'addNewLevel',
          value: function addNewLevel(idx) {
            if (idx < this.datasource.maxLevels - 1) this.target.levels.push('select level');
          }
        }, {
          key: 'removeLevels',
          value: function removeLevels(idx) {
            this.target.levels[idx + 1] = 'select level';
            if (idx < this.target.levels.length - 1) {
              var levelsToRemove = this.target.levels.length - idx - 2;
              while (levelsToRemove) {
                this.target.levels.pop();
                levelsToRemove--;
              }
            }
          }
        }, {
          key: 'buildHierarchy',
          value: function buildHierarchy(id) {

            var hierarchy = "?node=";
            //var hierarchy= "";
            var levelIdx;
            var len;

            //Builds hierarchy for the request
            if (this.target.levels.length > 1) {
              if (id != -1) len = id;else {
                len = this.target.levels.length - 1;
                //Don't consider the 'select level' form in building the hierarchy 
                if (this.target.levels.length == this.datasource.maxLevels && this.target.levels[this.target.levels.length - 1] != 'select level') len++;
              }

              for (levelIdx = 0; levelIdx < len; levelIdx++) {
                hierarchy += "/" + this.target.levels[levelIdx];
              }
            }

            //Check if request came from the sensor form
            if (id == -1)
              //hierarchy += "/sensor";
              hierarchy += ";sensors=true";

            return hierarchy;
          }
        }, {
          key: 'getOptions',
          value: function getOptions(query, idx) {
            var hierarchy = this.buildHierarchy(idx);
            return this.datasource.metricFindQuery(query, hierarchy || '');
          }
        }, {
          key: 'onChangeInternal',
          value: function onChangeInternal(idx) {

            //Add/remove new levels if the request did not come from the sensor form.
            if (idx != -1) {
              if (idx == this.target.levels.length - 1) this.addNewLevel(idx);else this.removeLevels(idx);
            }
            this.panelCtrl.refresh(); // Asks the panel to refresh data.
          }
        }, {
          key: 'toggleEditorMode',
          value: function toggleEditorMode() {
            this.target.rawQuery = !this.target.rawQuery;
          }
        }]);

        return GenericDatasourceQueryCtrl;
      }(QueryCtrl));

      _export('GenericDatasourceQueryCtrl', GenericDatasourceQueryCtrl);

      GenericDatasourceQueryCtrl.templateUrl = 'partials/query.editor.html';
    }
  };
});
//# sourceMappingURL=query_ctrl.js.map
