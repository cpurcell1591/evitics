var controller = {
	evList : document.getElementById("evList"),
	onEvListChange : function() {
		/* If new event */
		if(evList.value === '' &&
			evList[evList.selectedIndex].innerHTML === "New Event") {
			controller.newEvent();
		/* If neither new event nor --please selecxt-- */
		} else if(evList.value !== '') {
			$("#newevent-bar").remove();
			$("#go").show();
		}
	},
	newEvent : function() {
		template.get('newevent', function(data) {
			$("#go").hide();
			$('#template-content').append(data);
		});
	},
	go : function(evIndex) {
		if(typeof index === 'undefined') {
			index = evList.value;
		}
		EventAPI.current = EventAPI.eventList[index];
		EventAPI.current.checkedIn = [];
		EventAPI.current.id = index;
		template.goto('setup');
	}
};
$(controller.evList).change(controller.onEvListChange);