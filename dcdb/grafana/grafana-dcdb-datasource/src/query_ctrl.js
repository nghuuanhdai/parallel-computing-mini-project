import { QueryCtrl } from 'app/plugins/sdk';
import './css/query-editor.css!'

export class GenericDatasourceQueryCtrl extends QueryCtrl {

  constructor($scope, $injector) {
    super($scope, $injector);
    
    this.scope = $scope;
    this.target.levels = this.target.levels || ['select level'];

    //Sensor dropdown menu values
    this.target.sensor = this.target.sensor || 'select sensor';
    this.datasource.getMaxLevels();
    
  } 

  //Add a new level to the array.
  addNewLevel(idx) {
      if(idx < this.datasource.maxLevels - 1)
        this.target.levels.push('select level');
  }

  removeLevels(idx) {
    this.target.levels[idx + 1] = 'select level';
    if(idx < this.target.levels.length - 1) {
      var levelsToRemove = this.target.levels.length - idx - 2;
      while(levelsToRemove) {
        this.target.levels.pop();
        levelsToRemove--;
      }
    }
  }

  //Builds the hierachical query to be sent to the backend (e.g., /system/rack/chassis/sensor).
  buildHierarchy(id) {
    
    var hierarchy= "?node=";
    //var hierarchy= "";
    var levelIdx;
    var len;
    
    //Builds hierarchy for the request
    if(this.target.levels.length > 1) {
      if(id != -1)
        len = id;
      else {
        len = this.target.levels.length - 1;
        //Don't consider the 'select level' form in building the hierarchy 
        if((this.target.levels.length == this.datasource.maxLevels) && 
           (this.target.levels[this.target.levels.length - 1] != 'select level'))
          len++;
      }

      for(levelIdx = 0; levelIdx < len; levelIdx++) 
        hierarchy += "/" + this.target.levels[levelIdx]; 

    }

    //Check if request came from the sensor form
    if(id == -1)
      //hierarchy += "/sensor";
      hierarchy += ";sensors=true";
 
    return hierarchy;
  }

  getOptions(query, idx) {
    var hierarchy = this.buildHierarchy(idx);
    return this.datasource.metricFindQuery(query, hierarchy || '');
  }

  onChangeInternal(idx) {

    //Add/remove new levels if the request did not come from the sensor form.
    if(idx != -1) {
      if(idx == this.target.levels.length - 1)
        this.addNewLevel(idx);
      else
        this.removeLevels(idx);
    }
    this.panelCtrl.refresh(); // Asks the panel to refresh data.
  }

  toggleEditorMode() {
    this.target.rawQuery = !this.target.rawQuery;
  }

}

GenericDatasourceQueryCtrl.templateUrl = 'partials/query.editor.html';

