package com.yugabyte.yw.common;

import static play.mvc.Http.Status.BAD_REQUEST;
import static play.mvc.Http.Status.UNAUTHORIZED;

import com.google.inject.Inject;
import com.yugabyte.yw.common.config.GlobalConfKeys;
import com.yugabyte.yw.common.config.RuntimeConfGetter;
import com.yugabyte.yw.controllers.SessionController;
import com.yugabyte.yw.forms.CustomerLoginFormData;
import com.yugabyte.yw.models.Customer;
import com.yugabyte.yw.models.LdapDnToYbaRole;
import com.yugabyte.yw.models.Users;
import com.yugabyte.yw.models.Users.Role;
import io.ebean.DuplicateKeyException;
import java.nio.charset.Charset;
import java.util.Date;
import java.util.HashSet;
import java.util.List;
import java.util.Random;
import java.util.Set;
import java.util.UUID;
import lombok.AllArgsConstructor;
import lombok.Getter;
import lombok.Setter;
import lombok.extern.slf4j.Slf4j;
import org.apache.commons.lang3.StringUtils;
import org.apache.commons.lang3.tuple.ImmutableTriple;
import org.apache.commons.lang3.tuple.Triple;
import org.apache.directory.api.ldap.model.cursor.CursorException;
import org.apache.directory.api.ldap.model.cursor.EntryCursor;
import org.apache.directory.api.ldap.model.entry.Attribute;
import org.apache.directory.api.ldap.model.entry.Entry;
import org.apache.directory.api.ldap.model.entry.Value;
import org.apache.directory.api.ldap.model.exception.LdapAuthenticationException;
import org.apache.directory.api.ldap.model.exception.LdapException;
import org.apache.directory.api.ldap.model.exception.LdapNoSuchObjectException;
import org.apache.directory.api.ldap.model.message.SearchScope;
import org.apache.directory.ldap.client.api.LdapConnectionConfig;
import org.apache.directory.ldap.client.api.LdapNetworkConnection;
import org.apache.directory.ldap.client.api.NoVerificationTrustManager;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

@Slf4j
public class LdapUtil {

  public static final Logger LOG = LoggerFactory.getLogger(SessionController.class);
  public static final String windowsAdUserDoesNotExistErrorCode = "data 2030";
  public static final String USERNAME_KEYWORD = "{username}";

  @Inject private RuntimeConfGetter confGetter;

  @Getter
  @Setter
  @AllArgsConstructor
  public static class LdapConfiguration {
    String ldapUrl;
    Integer ldapPort;
    String ldapBaseDN;
    String ldapCustomerUUID;
    String ldapDnPrefix;
    boolean ldapUseSsl;
    boolean ldapUseTls;
    boolean useLdapSearchAndBind;
    String serviceAccountDistinguishedName;
    String serviceAccountPassword;
    String ldapSearchAttribute;
    boolean enableDetailedLogs;
    String ldapGroupSearchFilter;
    SearchScope ldapGroupSearchScope;
    String ldapGroupSearchBaseDn;
    String ldapGroupMemberOfAttribute;
    boolean ldapGroupUseQuery;
    boolean ldapGroupUseRoleMapping;
  }

  public Users loginWithLdap(CustomerLoginFormData data) throws LdapException {
    String ldapUrl = confGetter.getGlobalConf(GlobalConfKeys.ldapUrl);
    String getLdapPort = confGetter.getGlobalConf(GlobalConfKeys.ldapPort);
    Integer ldapPort = Integer.parseInt(getLdapPort);
    String ldapBaseDN = confGetter.getGlobalConf(GlobalConfKeys.ldapBaseDn);
    String ldapCustomerUUID = confGetter.getGlobalConf(GlobalConfKeys.ldapCustomerUUID);
    String ldapDnPrefix = confGetter.getGlobalConf(GlobalConfKeys.ldapDnPrefix);
    boolean ldapUseSsl = confGetter.getGlobalConf(GlobalConfKeys.enableLdap);
    boolean ldapUseTls = confGetter.getGlobalConf(GlobalConfKeys.enableLdapStartTls);
    boolean useLdapSearchAndBind = confGetter.getGlobalConf(GlobalConfKeys.ldapUseSearchAndBind);
    String serviceAccountDistiguishedName =
        confGetter.getGlobalConf(GlobalConfKeys.ldapServiceAccountDistinguishedName);
    String serviceAccountPassword =
        confGetter.getGlobalConf(GlobalConfKeys.ldapServiceAccountPassword);
    String ldapSearchAttribute = confGetter.getGlobalConf(GlobalConfKeys.ldapSearchAttribute);
    boolean enabledDetailedLogs = confGetter.getGlobalConf(GlobalConfKeys.enableDetailedLogs);
    String ldapGroupSearchFilter = confGetter.getGlobalConf(GlobalConfKeys.ldapGroupSearchFilter);
    SearchScope ldapGroupSearchScope =
        confGetter.getGlobalConf(GlobalConfKeys.ldapGroupSearchScope);
    String ldapGroupMemberOfAttribute =
        confGetter.getGlobalConf(GlobalConfKeys.ldapGroupMemberOfAttribute);
    boolean ldapGroupUseQuery = confGetter.getGlobalConf(GlobalConfKeys.ldapGroupUseQuery);
    boolean ldapGroupUseRoleMapping =
        confGetter.getGlobalConf(GlobalConfKeys.ldapGroupUseRoleMapping);
    String ldapGroupSearchBaseDn = confGetter.getGlobalConf(GlobalConfKeys.ldapGroupSearchBaseDn);

    LdapConfiguration ldapConfiguration =
        new LdapConfiguration(
            ldapUrl,
            ldapPort,
            ldapBaseDN,
            ldapCustomerUUID,
            ldapDnPrefix,
            ldapUseSsl,
            ldapUseTls,
            useLdapSearchAndBind,
            serviceAccountDistiguishedName,
            serviceAccountPassword,
            ldapSearchAttribute,
            enabledDetailedLogs,
            ldapGroupSearchFilter,
            ldapGroupSearchScope,
            ldapGroupSearchBaseDn,
            ldapGroupMemberOfAttribute,
            ldapGroupUseQuery,
            ldapGroupUseRoleMapping);
    Users user = authViaLDAP(data.getEmail(), data.getPassword(), ldapConfiguration);

    if (user == null) {
      return user;
    }

    try {
      user.save();
    } catch (DuplicateKeyException e) {
      log.info("User already exists.");
    }
    return user;
  }

  private UUID getCustomerUUID(String ldapCustomerUUID, String ybaUsername) {
    // check if old user
    Users oldUser = Users.find.query().where().eq("email", ybaUsername).findOne();
    if (oldUser != null && oldUser.getCustomerUUID() != null) {
      return oldUser.getCustomerUUID();
    }

    Customer cust = null;
    if (!ldapCustomerUUID.equals("")) {
      try {
        UUID custUUID = UUID.fromString(ldapCustomerUUID);
        cust = Customer.get(custUUID);
      } catch (Exception e) {
        throw new PlatformServiceException(
            BAD_REQUEST, "Customer UUID Specified is invalid. " + e.getMessage());
      }
    }

    if (cust == null) {
      List<Customer> allCustomers = Customer.getAll();
      if (allCustomers.size() != 1) {
        throw new PlatformServiceException(
            BAD_REQUEST, "Please specify ldap_customeruuid in Multi-Tenant Setup.");
      }
      cust = allCustomers.get(0);
    }

    return cust.getUuid();
  }

  private void deleteUserAndThrowException(String email) {
    Users.deleteUser(email);
    String errorMessage = "LDAP user " + email + " does not exist on the LDAP server";
    throw new PlatformServiceException(UNAUTHORIZED, errorMessage);
  }

  public LdapNetworkConnection createNewLdapConnection(LdapConnectionConfig ldapConnectionConfig) {
    return new LdapNetworkConnection(ldapConnectionConfig);
  }

  private Set<String> getGroups(
      String ybaUsername,
      Entry userEntry,
      LdapNetworkConnection connection,
      LdapConfiguration ldapConfiguration) {
    Set<String> groups = new HashSet<String>();

    if (!ldapConfiguration.isLdapGroupUseQuery()) {
      Attribute memberOf = userEntry.get(ldapConfiguration.getLdapGroupMemberOfAttribute());
      if (memberOf != null) {
        for (Value group : memberOf) {
          groups.add(group.getString());
        }
      }
      return groups;
    }

    String searchFilter = "";
    try {
      searchFilter =
          ldapConfiguration.getLdapGroupSearchFilter().replace(USERNAME_KEYWORD, ybaUsername);
      EntryCursor cursor =
          connection.search(
              ldapConfiguration.getLdapGroupSearchBaseDn(),
              searchFilter,
              ldapConfiguration.getLdapGroupSearchScope(),
              "*");
      while (cursor.next()) {
        Entry entry = cursor.get();
        groups.add(entry.getDn().toString());
      }
    } catch (LdapException | CursorException e) {
      e.printStackTrace();
      log.error(
          "Error querying groups with base dn: {} and search filter: {}",
          ldapConfiguration.getLdapBaseDN(),
          searchFilter);
    }

    return groups;
  }

  private Role getRoleMappedToLdapGroup(String group, UUID customerUuid) {
    Role role = null;
    LdapDnToYbaRole ldapDnToYbaRole =
        LdapDnToYbaRole.find
            .query()
            .where()
            .eq("distinguished_name", group)
            .eq("customer_uuid", customerUuid)
            .findOne();
    if (ldapDnToYbaRole != null) {
      role = ldapDnToYbaRole.ybaRole;
    }
    return role;
  }

  private Role getRoleFromGroupMappings(
      Entry userEntry,
      String ybaUsername,
      LdapNetworkConnection connection,
      LdapConfiguration ldapConfiguration,
      UUID customerUuid) {
    Role role = null;
    Set<String> groups = getGroups(ybaUsername, userEntry, connection, ldapConfiguration);
    for (String group : groups) {
      Role groupRole = getRoleMappedToLdapGroup(group, customerUuid);
      role = Role.union(role, groupRole);
    }
    return role;
  }

  private Triple<Entry, String, String> searchAndBind(
      String email,
      LdapConfiguration ldapConfiguration,
      LdapNetworkConnection connection,
      boolean enableDetailedLogs)
      throws Exception {
    Entry userEntry = null;
    String distinguishedName = "", role = "";

    try {
      connection.bind(
          ldapConfiguration.getServiceAccountDistinguishedName(),
          ldapConfiguration.getServiceAccountPassword());
    } catch (LdapAuthenticationException e) {
      String errorMessage = "Service Account bind failed. " + e.getMessage();
      log.error(errorMessage);
      throw new PlatformServiceException(UNAUTHORIZED, "Error binding to service account.");
    }
    try {
      EntryCursor cursor =
          connection.search(
              ldapConfiguration.getLdapBaseDN(),
              "(" + ldapConfiguration.getLdapSearchAttribute() + "=" + email + ")",
              SearchScope.SUBTREE,
              "*");
      log.info("Connection cursor: {}", cursor);
      while (cursor.next()) {
        userEntry = cursor.get();
        if (enableDetailedLogs) {
          log.info("LDAP server returned response: {}", userEntry.toString());
        }
        Attribute parseDn = userEntry.get("distinguishedName");
        log.info("parseDn: {}", parseDn);
        if (parseDn == null) {
          distinguishedName = userEntry.getDn().toString();
          log.info("parsedDn: {}", distinguishedName);
        } else {
          distinguishedName = parseDn.getString();
        }
        log.info("Distinguished name parsed: {}", distinguishedName);
        Attribute parseRole = userEntry.get("yugabytePlatformRole");
        if (parseRole != null) {
          role = parseRole.getString();
        }

        // Cursor.next returns true in some environments
        if (!StringUtils.isEmpty(distinguishedName)) {
          log.info("Successfully fetched DN");
          break;
        }
      }

      try {
        cursor.close();
        connection.unBind();
      } catch (Exception e) {
        log.error("Failed closing connections", e);
      }
    } catch (Exception e) {
      log.error("LDAP query failed.", e);
      throw new PlatformServiceException(BAD_REQUEST, "LDAP search failed.");
    }
    return new ImmutableTriple<Entry, String, String>(userEntry, distinguishedName, role);
  }

  public Users authViaLDAP(String email, String password, LdapConfiguration ldapConfiguration)
      throws LdapException {
    Users users = new Users();
    LdapNetworkConnection connection = null;
    try {
      LdapConnectionConfig config = new LdapConnectionConfig();
      config.setLdapHost(ldapConfiguration.getLdapUrl());
      config.setLdapPort(ldapConfiguration.getLdapPort());
      if (ldapConfiguration.isLdapUseSsl() || ldapConfiguration.isLdapUseTls()) {
        config.setTrustManagers(new NoVerificationTrustManager());
        if (ldapConfiguration.isLdapUseSsl()) {
          config.setUseSsl(true);
        } else {
          config.setUseTls(true);
        }
      }

      String distinguishedName =
          ldapConfiguration.getLdapDnPrefix() + email + "," + ldapConfiguration.getLdapBaseDN();
      connection = createNewLdapConnection(config);

      String role = "";
      Entry userEntry = null;
      if (ldapConfiguration.isUseLdapSearchAndBind()) {
        if (ldapConfiguration.getServiceAccountDistinguishedName().isEmpty()
            || ldapConfiguration.getServiceAccountPassword().isEmpty()
            || ldapConfiguration.getLdapSearchAttribute().isEmpty()) {
          throw new PlatformServiceException(
              BAD_REQUEST,
              "Service account and LDAP Search Attribute must be configured"
                  + " to use search and bind.");
        }
        Triple<Entry, String, String> entryDnRole =
            searchAndBind(
                email, ldapConfiguration, connection, ldapConfiguration.isEnableDetailedLogs());
        userEntry = entryDnRole.getLeft();
        String fetchedDistinguishedName = entryDnRole.getMiddle();
        if (!fetchedDistinguishedName.isEmpty()) {
          distinguishedName = fetchedDistinguishedName;
        }
        role = entryDnRole.getRight();
      }

      email = email.toLowerCase();
      try {
        connection.bind(distinguishedName, password);
      } catch (LdapNoSuchObjectException e) {
        log.error(e.getMessage());
        deleteUserAndThrowException(email);
      } catch (LdapAuthenticationException e) {
        log.error(e.getMessage());
        if (e.getMessage().contains(windowsAdUserDoesNotExistErrorCode)) {
          deleteUserAndThrowException(email);
        }
        String errorMessage = "Failed with " + e.getMessage();
        throw new PlatformServiceException(UNAUTHORIZED, errorMessage);
      }

      // User has been authenticated.
      users.setCustomerUUID(getCustomerUUID(ldapConfiguration.getLdapCustomerUUID(), email));

      if (role.isEmpty() && !ldapConfiguration.isUseLdapSearchAndBind()) {
        if (!ldapConfiguration.getServiceAccountDistinguishedName().isEmpty()
            && !ldapConfiguration.getServiceAccountPassword().isEmpty()) {
          connection.unBind();
          try {
            connection.bind(
                ldapConfiguration.getServiceAccountDistinguishedName(),
                ldapConfiguration.getServiceAccountPassword());
          } catch (LdapAuthenticationException e) {
            String errorMessage =
                "Service Account bind failed. "
                    + "Defaulting to current user connection with LDAP Server."
                    + e.getMessage();
            log.error(errorMessage);
            connection.bind(distinguishedName, password);
          }
        }

        try {
          EntryCursor cursor =
              connection.search(distinguishedName, "(objectclass=*)", SearchScope.SUBTREE, "*");
          while (cursor.next()) {
            userEntry = cursor.get();
            Attribute parseRole = userEntry.get("yugabytePlatformRole");
            role = parseRole.getString();
          }
        } catch (Exception e) {
          log.debug(
              String.format("LDAP query for yugabytePlatformRole failed with %s", e.getMessage()));
        }
      }

      if (ldapConfiguration.isLdapGroupUseRoleMapping()) {
        if (ldapConfiguration.getServiceAccountDistinguishedName().isEmpty()
            || ldapConfiguration.getServiceAccountPassword().isEmpty()) {
          throw new PlatformServiceException(
              BAD_REQUEST, "Service account must be configured to use group to role mapping.");
        }
        connection.unBind();
        try {
          connection.bind(
              ldapConfiguration.getServiceAccountDistinguishedName(),
              ldapConfiguration.getServiceAccountPassword());
        } catch (LdapAuthenticationException e) {
          String errorMessage = "Service Account bind failed. " + e.getMessage();
          log.error(errorMessage);
          throw new PlatformServiceException(
              UNAUTHORIZED,
              "Error binding to service account. Cannot get group memberships for username: "
                  + email);
        }

        Role roleFromGroupMappings =
            getRoleFromGroupMappings(
                userEntry, email, connection, ldapConfiguration, users.getCustomerUUID());

        if (roleFromGroupMappings == null) {
          log.warn("No role mappings from LDAP group membership of user: " + email);
        }

        if (role.isEmpty()) {
          if (roleFromGroupMappings != null) {
            role = roleFromGroupMappings.toString();
            log.info(
                "No role found from yugabytePlatformRole, "
                    + "using role {} found from LDAP group mappings for user: {}",
                role,
                email);
          } else {
            log.warn(
                "No role found from either yugabytePlatformRole or "
                    + "LDAP group mapping for LDAP user: {}",
                email);
          }
        } else {
          try {
            Role roleEnum = Role.valueOf(role);
            role = Role.union(roleEnum, roleFromGroupMappings).toString();
            log.info(
                "Roles from yugabytePlatformRole and LDAP group memberships for LDAP user: "
                    + "{} combined to assign {}",
                email,
                role);
          } catch (IllegalArgumentException e) {
            if (roleFromGroupMappings != null) {
              log.error(
                  "Invalid role: {} obtained from yugabytePlatformRole,"
                      + " attempting to use role mapped to LDAP groups: {}",
                  role,
                  roleFromGroupMappings);
              role = roleFromGroupMappings.toString();
            } else {
              log.error(
                  "Invalid role: \"{}\" obtained from yugabytePlatformRole, "
                      + "role mapped to LDAP groups is also null.",
                  role);
            }
          }
        }
      }

      Users.Role roleToAssign;
      users.setLdapSpecifiedRole(true);
      switch (role) {
        case "Admin":
          roleToAssign = Users.Role.Admin;
          break;
        case "SuperAdmin":
          roleToAssign = Users.Role.SuperAdmin;
          break;
        case "BackupAdmin":
          roleToAssign = Users.Role.BackupAdmin;
          break;
        case "ReadOnly":
          roleToAssign = Users.Role.ReadOnly;
          break;
        default:
          roleToAssign = Users.Role.ReadOnly;
          log.warn("No valid role could be ascertained, defaulting to ReadOnly.");
          if (!ldapConfiguration.isLdapGroupUseRoleMapping()) {
            users.setLdapSpecifiedRole(false);
          }
      }
      Users oldUser = Users.find.query().where().eq("email", email).findOne();
      if (oldUser != null) {
        oldUser.setLdapSpecifiedRole(users.isLdapSpecifiedRole());
      }

      if (oldUser != null
          && (oldUser.getRole() == roleToAssign || !oldUser.isLdapSpecifiedRole())) {
        return oldUser;
      } else if (oldUser != null && (oldUser.getRole() != roleToAssign)) {
        oldUser.setRole(roleToAssign);
        return oldUser;
      } else {
        users.setEmail(email.toLowerCase());
        byte[] passwordLdap = new byte[16];
        new Random().nextBytes(passwordLdap);
        String generatedPassword = new String(passwordLdap, Charset.forName("UTF-8"));
        users.setPassword(generatedPassword); // Password is not used.
        users.setUserType(Users.UserType.ldap);
        users.setCreationDate(new Date());
        users.setPrimary(false);
        users.setRole(roleToAssign);
      }
    } catch (LdapException e) {
      LOG.error("LDAP error while attempting to auth email {}", email);
      LOG.debug(e.getMessage());
      String errorMessage = "LDAP parameters are not configured correctly. " + e.getMessage();
      throw new PlatformServiceException(BAD_REQUEST, errorMessage);
    } catch (Exception e) {
      LOG.error("Failed to authenticate with LDAP for email {}", email);
      LOG.debug(e.getMessage());
      String errorMessage = "Invalid LDAP credentials. " + e.getMessage();
      throw new PlatformServiceException(UNAUTHORIZED, errorMessage);
    } finally {
      if (connection != null) {
        connection.unBind();
        connection.close();
      }
    }
    return users;
  }
}
