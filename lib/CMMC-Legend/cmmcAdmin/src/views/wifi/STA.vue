<template>
  <div class="content is-fluid">
    <section class="section">
      <div class="columns">
        <div class="column">
          <div class="heading">
            <h1 class="title">STA Configuration</h1>
          </div>
          <div v-if="server_response" class="notification is-primary">
            {{ server_response }}
          </div>
          <div class="control">
            <label v-show="toggle" class="label">Select SSID</label>
            <div v-show="toggle" class="select control has-icon is-fluid">
              <i class="fa fa-wifi"></i>
              <select v-bind:disabled="disable_submit" v-model="select_sta_ssid" style="padding-left: 2.25em">
                <option v-for="option in output" v-bind:value="option">
                  {{ option }}
                </option>
              </select>
            </div>
            <button v-on:click.stop="reloadAP" v-show="toggle" v-bind:class="{ 'is-loading': isLoadAPList }"
                    class="button is-warning">Refresh
            </button>


            <label v-show="!toggle" class="label">Manual SSID</label>
            <p class="control has-icon" v-show="!toggle">
              <input v-model="manual_sta_ssid" class="input" type="text" placeholder="SSID">
              <i class="fa fa-wifi"></i>
            </p>

            <div class="control is-half">
              <label class="label">Password</label>
              <p class="control has-icon">
                <input v-bind:disabled="disable_submit" v-model="sta_password" class="input" type="text" placeholder="Password">
                <i class="fa fa-lock"></i>
              </p>
            </div>
            <p class="control">
              <button v-bind:disabled="disable_submit" class="button is-primary" v-on:click.stop="onSubmit">Submit</button>
              <!--              <button class="button" v-on:click.stop="onRunMode">Manual SSID</button>-->
              <button v-show="toggle" class="button" v-on:click="toggle= !toggle">Manual SSID</button>
              <button v-show="!toggle" class="button is-warning" v-on:click="toggle= !toggle">Select SSID</button>
            </p>
            <!--            hi {{ ssid }}-->
          </div>
        </div>
      </div>
    </section>
  </div>
</template>

<script>
  import {saveSTAConfig, getSTAConfig} from "../../api";

  export default {
    components: {},
    props: {},
    mounted() {
      this.reloadAP();
    },
    methods: {
      reloadAP() {
        this.isLoadAPList = true;
        this.disable_submit = true;
        getSTAConfig(this).then((json) => {
          setTimeout(() => {
            this.ap_list = [...this.ap_list, ...json];
            this.ap_list.forEach((val, idx) => {
              this.output[val.ssid] = val.ssid;
            });

            this.output = Object.keys(this.output).sort(function(a, b) {
              if (a < b) { return -1; }
              if (a > b) { return 1; }
              return 0;
            });
            this.select_sta_ssid = this.output[0];
            this.sta_password = "";
            this.isLoadAPList = false;
            this.disable_submit = false;
          }, 1);
        })
          .catch((err) => {
            this.isLoadAPList = false;
          });
      },
      onSubmit() {
        let context = this;
        context.ssid = "";
        context.disable_submit = true;
        if (!context.toggle) {
          context.ssid = context.manual_sta_ssid;
        } else {
          context.ssid = context.select_sta_ssid;
        }
        setTimeout(() => {
          saveSTAConfig(context, context.ssid, context.sta_password)
            .then((resp) => {
              context.disable_submit = false;
              context.server_response = JSON.stringify(resp);
              context.saving = false;
            })
            .catch((err) => {
              context.disable_submit = false;
              context.saving = false;
            });

        }, 1);
      },
      onRunMode() {
        let c = confirm("Confirm to reboot?");
        let context = this;
        if (c) {
          context.$http.get("/enable")
            .then((response) => {
              context.server_response = response.text();
            });
        }
      }
    },
    data() {
      return {
        toggle: true,
        server_response: "",
        select_sta_ssid: null,
        manual_sta_ssid: null,
        sta_password: null,
        disable_submit: true,
        ap_list: [],
        output: {},
        isLoadAPList: true,
        ssid: ""
      };
    },
    computed: {
      now: function() {
        return Date.now();
      }
    },
    created() {

    }
  };
</script>

<style scoped>
  .button {
    margin: 5px 0 0;
  }

  .control .button {
    margin: inherit;
  }
</style>
